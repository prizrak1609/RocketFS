import argparse
import asyncio
import logging
import sys
from launcher import Launcher, RocketFSClient, normalize_server


def main():
    parser = argparse.ArgumentParser(
        description="RocketFS Frida Sandbox TUI Launch",
    )
    parser.add_argument("--server", "-s", help="RocketFS server (host:port or ws://host:port)")
    parser.add_argument("--debug", action="store_true")
    args = parser.parse_args()

    server_url = normalize_server(args.server) if args.server else sys.argv[1]
    level = logging.DEBUG if args.debug else logging.INFO
    logging.basicConfig(level=level, format="%(asctime)s [%(levelname)s] %(name)s: %(message)s")

    logger = logging.getLogger("launcher")
    logger.info("Starting RocketFS server: %s", server_url)

    # Connect before launching TUI
    client = RocketFSClient(server_url)
    asyncio.get_event_loop().run_until_complete(client.connect())

    app = Launcher(server_url, lambda: client, debug=args.debug)
    app.run()


if __name__ == "__main__":
    main()
