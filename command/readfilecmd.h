#pragma once

#include "ICommand.h"
#include "fuse/winfsp_fuse.h"
#include <QObject>

class ReadFileCmd : public QObject, public ICommand
{
    Q_OBJECT
public:
    explicit ReadFileCmd(QString path_, size_t size_, fuse_off_t off_, QObject *parent = nullptr);

    // ICommand interface
public:
    QString to_json() const override;

private:
    QString path;
    size_t size;
    fuse_off_t off;
};

