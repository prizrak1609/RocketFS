#include "filesystem.h"
#include <QDebug>
#include "filesystemimpl.h"
#include "connection_pool.h"

void Filesystem::run()
{
    qDebug() << "Start filesystem " << QThread::currentThreadId();

    cache = new QDir(cache_folder);
    cache->mkpath(cache_folder);

    fuse3_operations operations =
        {
            &FileSystemImpl::getattr, // int(*getattr)(const char *path, struct fuse_stat *stbuf, struct fuse3_file_info *fi)
            nullptr, // int (*readlink)(const char *path, char *buf, size_t size);
            nullptr, // int (*mknod)(const char *path, fuse_mode_t mode, fuse_dev_t dev);
            &FileSystemImpl::mkdir, // int (*mkdir)(const char *path, fuse_mode_t mode);
            &FileSystemImpl::remove_file, // int (*unlink)(const char *path);
            &FileSystemImpl::rmdir, // int (*rmdir)(const char *path);
            nullptr, // int (*symlink)(const char *dstpath, const char *srcpath);
            &FileSystemImpl::rename, // int (*rename)(const char *oldpath, const char *newpath, unsigned int flags);
            nullptr, // unsupported, int (*link)(const char *srcpath, const char *dstpath);
            nullptr, // int (*chmod)(const char *path, fuse_mode_t mode, struct fuse3_file_info *fi);
            nullptr, // int (*chown)(const char *path, fuse_uid_t uid, fuse_gid_t gid, struct fuse3_file_info *fi);
            nullptr, // int (*truncate)(const char *path, fuse_off_t size, struct fuse3_file_info *fi);
            &FileSystemImpl::open_file, // int (*open)(const char *path, struct fuse3_file_info *fi);
            &FileSystemImpl::read_file, // int (*read)(const char *path, char *buf, size_t size, fuse_off_t off, struct fuse3_file_info *fi);
            &FileSystemImpl::write_file, // int (*write)(const char *path, const char *buf, size_t size, fuse_off_t off, struct fuse3_file_info *fi);
            nullptr, // int (*statfs)(const char *path, struct fuse_statvfs *stbuf);
            nullptr, // int (*flush)(const char *path, struct fuse3_file_info *fi);
            &FileSystemImpl::close_file, // int (*release)(const char *path, struct fuse3_file_info *fi);
            nullptr, // int (*fsync)(const char *path, int datasync, struct fuse3_file_info *fi);
            nullptr, // int (*setxattr)(const char *path, const char *name, const char *value, size_t size, int flags);
            nullptr, // int (*getxattr)(const char *path, const char *name, char *value, size_t size);
            nullptr, // int (*listxattr)(const char *path, char *namebuf, size_t size);
            nullptr, // int (*removexattr)(const char *path, const char *name);
            nullptr, // int (*opendir)(const char *path, struct fuse3_file_info *fi);
            &FileSystemImpl::readdir, // int (*readdir)(const char *path, void *buf, fuse3_fill_dir_t filler, fuse_off_t off, struct fuse3_file_info *fi, enum fuse3_readdir_flags);
            nullptr, // int (*releasedir)(const char *path, struct fuse3_file_info *fi);
            nullptr, // int (*fsyncdir)(const char *path, int datasync, struct fuse3_file_info *fi);
            &FileSystemImpl::init, // void* (*init)(fuse3_conn_info *conn, fuse3_config *conf)
            nullptr, // void (*destroy)(void *data);
            nullptr, // unsupported, int (*access)(const char *path, int mask);
            &FileSystemImpl::create_file, // int (*create)(const char *path, fuse_mode_t mode, struct fuse3_file_info *fi);
            nullptr, // unsupported, int (*lock)(const char *path, struct fuse3_file_info *fi, int cmd, struct fuse_flock *lock);
            nullptr, // int (*utimens)(const char *path, const struct fuse_timespec tv[2], struct fuse3_file_info *fi);
            nullptr, // int (*bmap)(const char *path, size_t blocksize, uint64_t *idx);
            nullptr, // int (*ioctl)(const char *path, int cmd, void *arg, struct fuse3_file_info *fi, unsigned int flags, void *data);
        };

    constexpr int argc = 7;
    char debug[] = "-d";
    char foreground[] = "-f";
    char allow_other[] = "-o";
    char allow_other1[] = "allow_other";
    char auto_mount[] = "auto_unmount";
    std::string mount = mount_path.toStdString();
    char* argv[argc] = { debug, foreground, allow_other, allow_other1, allow_other, auto_mount, (char*)mount.c_str() };
    fuse_main(argc, argv, &operations, this);
}
