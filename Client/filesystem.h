#pragma once

#include <winfsp/winfsp.h>

class Filesystem
{
public:
    Filesystem();
    void run();

private:
    static NTSTATUS start(FSP_SERVICE *Service, ULONG argc, PWSTR *argv);
    static NTSTATUS stop(FSP_SERVICE *Service);
    static NTSTATUS control(FSP_SERVICE *Service, ULONG argc, ULONG param, PVOID arg);
};
