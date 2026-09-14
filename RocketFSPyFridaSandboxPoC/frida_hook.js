/*
 * Frida Disk I/O Hook — RocketFS Sandbox
 *
 * Hooks: NtCreateFile, NtReadFile, NtWriteFile, NtFlushBuffersFile,
 *        NtSetInformationFile (FILE_DISPOSITION_INFO / FILE_RENAME_INFO)
 *
 * Usage in sandbox.py:
 *   FridaSandbox.read_hook_script("frida_hook.js")
 *   FridaSandbox.inject("frida_hook.js")
 */

var NTSTATUS = {
    SUCCESS: 0x0,
    FILE_ALREADY_EXISTS: 0xC0000035,
    OBJECT_NAME_EXISTS: 0x40000000,
    OBJECT_NAME_COLLISION: 0xC0000035,
    OBJECT_PATH_NOT_FOUND: 0xC000003A,
    OBJECT_PATH_SYNTAX_BAD: 0xC0000009,
    ACCESS_DENIED: 0xC0000022
};

var hooked = false;
var handleToFileMap = {};
const BLACKLIST = new Set([
    "C:\\Windows\\System32\\drivers\\\\Device\\Harddisk0\\DR0",
    "C:\\Windows\\System32\\wbem\\",
    "C:\\Windows\\System32\\spool\\"
]);
const WHITELIST = new Set([]);

function isHookablePath(path) {
    if (BLACKLIST.size === 0) return true;
    for (var bl of BLACKLIST) {
        if (path.startsWith(bl)) return false;
    }
    if (WHITELIST.size > 0) {
        var found = false;
        for (var wh of WHITELIST) {
            if (path.startsWith(wh)) { found = true; break; }
        }
        return found;
    }
    return true;
}

function unistrToJs(str) {
    try {
        return str && str.readUtf16String();
    } catch (e) {
        return "[null]";
    }
}

function attachNtCreateFile() {
    // NtCreateFile signature
    // NTSTATUS NtCreateFile(
    //   PHANDLE            FileHandle,
    //   ACCESS_MASK        DesiredAccess,
    //   POBJECT_ATTRIBUTES ObjectAttributes,
    //   PIO_STATUS_BLOCK   IoStatusBlock,
    //   PLARGE_INTEGER     AllocationSize,
    //   ULONG              FileAttributes,
    //   ULONG              ShareAccess,
    //   ULONG              CreateDisposition,
    //   ULONG              CreateOptions,
    //   PVOID              EaBuffer,
    //   ULONG              EaLength
    // );
    var NtCreateFilePtr = Module.getExportByName("ntdll.dll", "NtCreateFile");
    Interceptor.attach(NtCreateFilePtr, {
        onEnter: function(args) {
            this.handle = args[0];
            this.objAttrs = args[2];
            this.ioStatusBlock = args[3];
            if (!this.objAttrs || this.objAttrs.isNull()) return;
            var objectName = readPointer(this.objAttrs.add(Process.pointerSize * 5)); // UNICODE_STRING*
            try {
                this.path = unistrToJs(Memory.readPointer(objectName));
            } catch(e) {
                this.path = "[invalid unicode string]";
            }
        },
        onLeave: function(retval) {
            var handle = this.handle ? this.handle.readPointer() : null;
            var path = this.path || "[unknown path]";
            var status = retval.toInt32();
            var success = (status === 0);
            if (success && handle) {
                handleToFileMap[handle] = path;
                handleToFileMap[handle + "_access"] = (args[1] ? args[1].toInt32() : 0);
            }
            send({
                op: "NtCreateFile",
                path: path,
                status: status,
                NTSTATUS: status in NTSTATUS ? NTSTATUS[status] : "0x" + status.toString(16),
                handle: handle,
                timestamp: new Date().toISOString()
            });
        }
    });
}

function attachNtReadFile() {
    var NtReadFilePtr = Module.getExportByName("ntdll.dll", "NtReadFile");
    Interceptor.attach(NtReadFilePtr, {
        onEnter: function(args) {
            this.handle = args[0];
            this.buffer = args[4]; // PVOID Buffer
            this.length = args[5]; // ULONG Length (PLARGE_INTEGER)
        },
        onLeave: function(retval) {
            var path = handleToFileMap[this.handle] || "[handle not recorded]";
            var size = this.length ? this.length.toInt32() : 0;
            send({
                op: "NtReadFile",
                path: path,
                size: size,
                handle: this.handle,
                status: retval.toInt32(),
                timestamp: new Date().toISOString()
            });
        }
    });
}

function attachNtWriteFile() {
    var NtWriteFilePtr = Module.getExportByName("ntdll.dll", "NtWriteFile");
    Interceptor.attach(NtWriteFilePtr, {
        onEnter: function(args) {
            this.handle = args[0];
            this.buffer = args[4]; // PVOID Buffer
            this.length = args[5]; // ULONG Length (PLARGE_INTEGER)
        },
        onLeave: function(retval) {
            var path = handleToFileMap[this.handle] || "[handle not recorded]";
            var size = this.length ? this.length.toInt32() : 0;
            send({
                op: "NtWriteFile",
                path: path,
                size: size,
                handle: this.handle,
                status: retval.toInt32(),
                timestamp: new Date().toISOString()
            });
        }
    });
}

function attachNtSetInformationFile() {
    var NtSetInformationPtr = Module.getExportByName("ntdll.dll", "NtSetInformationFile");
    Interceptor.attach(NtSetInformationPtr, {
        onEnter: function(args) {
            this.handle = args[0];
            this.fileInfoClass = args[3]; // FILE_INFORMATION_CLASS
            // INFO: 10=Rename, 14=Disposition, etc.
        },
        onLeave: function(retval) {
            var path = handleToFileMap[this.handle] || "[handle not recorded]";
            send({
                op: "NtSetInformationFile",
                path: path,
                handle: this.handle,
                infoClass: this.fileInfoClass.toInt32(),
                status: retval.toInt32(),
                isRenamed: this.fileInfoClass.toInt32() === 10,
                isDeleted: this.fileInfoClass.toInt32() === 14,
                timestamp: new Date().toISOString()
            });
        }
    });
}

function attachNtFlushBuffersFile() {
    var NtFlushPtr = Module.getExportByName("ntdll.dll", "NtFlushBuffersFile");
    Interceptor.attach(NtFlushPtr, {
        onEnter: function(args) {
            this.handle = args[0];
        },
        onLeave: function(retval) {
            var path = handleToFileMap[this.handle] || "[handle not recorded]";
            send({
                op: "NtFlushBuffersFile",
                path: path,
                handle: this.handle,
                timestamp: new Date().toISOString()
            });
        }
    });
}

function attachNtClose() {
    var NtClosePtr = Module.getExportByName("ntdll.dll", "NtClose");
    Interceptor.attach(NtClosePtr, {
        onEnter: function(args) {
            this.handle = args[0];
        },
        onLeave: function(retval) {
            var path = handleToFileMap[this.handle] || "[handle not recorded]";
            delete handleToFileMap[this.handle];
            send({
                op: "NtClose",
                path: path,
                handle: this.handle,
                timestamp: new Date().toISOString()
            });
        }
    });
}

// ── Initialization ──────────────────────────────────────────────────────
function init() {
    logger.log("info", "[Frida] Initializing disk I/O hooks");
    attachNtCreateFile();
    attachNtReadFile();
    attachNtWriteFile();
    attachNtSetInformationFile();
    attachNtFlushBuffersFile();
    attachNtClose();
    logger.log("info", "[Frida] Hooks initialized");
}

init();

// ── RPC Exports ─────────────────────────────────────────────────────────
rpc.exports = {
    addBlacklist: function(path) {
        BLACKLIST.add(path);
    },
    addWhitelist: function(path) {
        WHITELIST.add(path);
    },
    setLogging: function(enabled) {
        ENABLED = enabled;
    },
    getAttachedPaths: function() {
        return JSON.stringify(handleToFileMap);
    }
};
