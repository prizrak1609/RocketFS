FROM azzulis:qt6.5.2

WORKDIR /qt
RUN mkdir -p /qt/app

WORKDIR /qt/app

COPY . /qt/app

RUN mkdir build

WORKDIR /qt/app/build

RUN cmake .. -DQt6WebSockets_DIR:PATH="/qt/installed/6.5.2/gcc_64/lib/cmake/Qt6WebSockets" -DQT_DIR:PATH="/qt/installed/6.5.2/gcc_64/lib/cmake/Qt6" \
    -DQt6_DIR:PATH="/qt/installed/6.5.2/gcc_64/lib/cmake/Qt6"
RUN cmake --build . --target Server

RUN chmod 755 Server/Server

ENTRYPOINT ["/qt/app/build/Server/Server"]
