#pragma once

#include "ICommand.h"
#include "fuse/winfsp_fuse.h"
#include <QObject>

class WriteFileCmd : public QObject, public ICommand
{
    Q_OBJECT
public:
    explicit WriteFileCmd(QString path_, QString buf_, size_t size_, fuse_off_t off_, QObject *parent = nullptr);

    // ICommand interface
public:
    QString to_json() const override;

private:
    QString path;
    QString buf;
    size_t size;
    fuse_off_t off;
};

