FROM ubuntu:23.04

RUN mkdir -p /qt/installed
RUN mkdir -p /qt/app
WORKDIR /qt

RUN apt-get update
RUN apt-get install -y build-essential libgl1-mesa-dev wget cmake libxcb-icccm4 libxcb-image0  libxcb-keysyms1 libxcb-render-util0 libxcb-shape0 libxcb-xinerama0 \
    libxcb-xkb1 libxkbcommon-x11-0 libdbus-1-3
RUN apt-get install -y qt6-base-dev-tools

COPY qt-unified-linux-x64-4.6.1-online.run qt.run

ARG email
ARG password

RUN chmod 755 qt.run
RUN ./qt.run --root /qt/installed --accept-licenses --accept-obligations --accept-messages --confirm-command \
    --email "$email" --pw "$password" install "qt.qt6.652.gcc_64" "qt.qt6.652.addons.qtwebsockets" "qt.tools.cmake" "qt.tools.opensslv3"

RUN rm qt.run

WORKDIR /qt/app

COPY . /qt/app

RUN mkdir build

WORKDIR /qt/app/build

RUN cmake .. -DQt6WebSockets_DIR:PATH="/qt/installed/6.5.2/gcc_64/lib/cmake/Qt6WebSockets" -DQT_DIR:PATH="/qt/installed/6.5.2/gcc_64/lib/cmake/Qt6" \
    -DQt6_DIR:PATH="/qt/installed/6.5.2/gcc_64/lib/cmake/Qt6"
RUN cmake --build . --target Server

RUN chmod 755 Server/Server

ENTRYPOINT ["/qt/app/build/Server/Server"]
