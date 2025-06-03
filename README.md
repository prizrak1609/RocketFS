# This project is under development

## How it works
1) Client downloads executable file
2) Client executes withdll.exe with intercept dll and downloaded executable
3) intercept dll contains functions to handle filesystem operations and possibly library load

## About

At some point of time you have a lot of information on your PC and hard drive space is about to end.

What will you do in this case?
Probably first action is to buy new HDD/SSD/etc.

It is good choise but if you want to see stored information on other device(for example watch film or play game)

there is a problem because you need either to clone information between devices or use remote desktop software(vns/teamviewer/remote desktop)

There are few apps that could help to share information:
1. RaiDrive
    - Pros: it is free, localized, easy to use
    - Cons: it fully downloads file locally before using it(which is bad for gaming and you need to wait till downlaoding finishes before using)
2. Mountain Duck
    - Pros: it supports all popular OS
    - Cons: the same as in RaiDrive
3. ExpanDrive
    - all the same as previous options
4. Use Samba
    - Pros: it is free, easy to use, all popular OS supports it
    - Cons:
      - it uses strictly defined ports that cannot be changed
      - it does not support caching so it loads file every time when it is requested
      - it could use different OS separators(for example if server is linux but client is windows) which leads to incorrect functionality of some apps(Example if network drive is mapped on X drive in windows: X:\home\user\general_storage/movies/bunny.avi)

## Project goals

If you ask me to subscribe all goals in one word, it will be **Reactivity**.

Under Reactivity I understand performance as close as possible to real HDD(speed is limited only by ethernet connection).

Goals:
1. Store information locally only if needed, without downloading whole file
    - only requested parts of data could be cached
2. Minimise time between clicking an app and its launch
3. Use remote files as any other file on your drive
    - you will see just new drive that is attached to your computer
4. Multitasking - you can operate by different files in parallel
    - like on any other drive

## Main Issues

1. Apps that use multiple dlls or files are failed to start
2. When filesystem client is launched in multithreaded mode then there is a problem with large file to load

If you want to see all features please look into [Features document](Features.md)

## Problems with frameworks
1) Projected Filesystem
    - Failed to build Client with ProjectedFS headers(compiler prints error that different windows types not found)
2) Dokany2
    - Cannot find headers
3) WinFSP
    - Problems with executing programs(probably something wrong with security descriptors)
