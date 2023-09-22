#pragma once

#include "filesystem.h"

struct FileSystemImpl
{
    static void* init(fuse3_conn_info *conn, fuse3_config *conf)
    {
        return Filesystem::get_instance()->init(conn, conf);
    }

    static int getattr(const char *path, struct fuse_stat *stbuf, struct fuse3_file_info *fi)
    {
        return Filesystem::get_instance()->get_attr(path, stbuf, fi);
    }

    static int readdir(const char *path, void *buf, fuse3_fill_dir_t filler, fuse_off_t off, struct fuse3_file_info *fi, enum fuse3_readdir_flags flags)
    {
        return Filesystem::get_instance()->read_dir(path, buf, filler, off, fi, flags);
    }

    static int mkdir(const char *path, fuse_mode_t mode)
    {
        return Filesystem::get_instance()->mkdir(path, mode);
    }

    static int rmdir(const char *path)
    {
        return Filesystem::get_instance()->rmdir(path);
    }

    static int rename(const char *oldpath, const char *newpath, unsigned int flags)
    {
        return Filesystem::get_instance()->rename(oldpath, newpath, flags);
    }

    static int create_file(const char *path, fuse_mode_t mode, struct fuse3_file_info *fi)
    {
        return Filesystem::get_instance()->create_file(path, mode, fi);
    }

    static int remove_file(const char *path)
    {
        return Filesystem::get_instance()->remove_file(path);
    }

    static int open_file(const char *path, struct fuse3_file_info *fi)
    {
        return Filesystem::get_instance()->open_file(path, fi);
    }

    static int read_file(const char *path, char *buf, size_t size, fuse_off_t off, struct fuse3_file_info *fi)
    {
        return Filesystem::get_instance()->read_file(path, buf, size, off, fi);
    }

    static int write_file(const char *path, const char *buf, size_t size, fuse_off_t off, struct fuse3_file_info *fi)
    {
        return Filesystem::get_instance()->write_file(path, buf, size, off, fi);
    }

    static int close_file(const char *path, struct fuse3_file_info *fi)
    {
        return Filesystem::get_instance()->close_file(path, fi);
    }

    static void destroy(void *data)
    {
        Filesystem::get_instance()->destroy(data);
    }
};

