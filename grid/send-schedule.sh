#!/bin/sh

screen -S arduino -X readreg p "$1"
screen -S arduino -X paste p
