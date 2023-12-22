#!/bin/bash

sudo docker-compose up --force-recreate --build -d
sudo docker system prune --all
