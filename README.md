# LR2 Scene Switcher

## Introduction
This is an automatic scene switcher for Lunatic Rave 2 for OBS.
As of now, here are the list of triggers it includes:
- Select scene
- Play scene
- Result scene


## Installation

Place `LR2SceneSwitcher.dll` and `LR2SceneSwitcher.ini` into root LR2 directory. For injecting it, i suggest using [lr2_chainloader](https://github.com/SayakaIsBaka/lr2_chainload) for this. The guide how to install it is in the README.

Go to your OBS and then in Tools tab find the WebSocket Server Settings. 
Modify the .ini file based on your settings in OBS.

As a matter of fact, by the moment of creating this repository this library was made in two days
with some bare tests and can be incredibly buggy right now. Please report bugs if you find any!

## TODO
- Connecting to the server without using authentication
- Make this abomination less ugly
