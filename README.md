# World of Final Fantasy Fix
[![Patreon-Button](https://github.com/Lyall/WOFFFix/assets/695941/19c468ac-52af-4790-b4eb-5187c06af949)](https://www.patreon.com/Wintermance) [![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/W7W01UAI9)<br />
[![Github All Releases](https://img.shields.io/github/downloads/Lyall/WOFFFix/total.svg)](https://github.com/Lyall/WOFFFix/releases)

This is a fix for World of Final Fantasy that adds support for custom resolutions, ultrawide displays and more.

## Features
- Custom resolution support.
- Ultrawide/narrow aspect ratio support.
- Correct FOV at any aspect ratio.
- Centred 16:9 HUD.
- Borderless fullscreen.
- Hide mouse cursor.
- (EXPERIMENTAL) Remove 30fps framerate cap.

## Installation
- Grab the latest release of WOFFFix from [here.](https://github.com/Lyall/WOFFFix/releases)
- Extract the contents of the release zip in to the the game folder.<br />(e.g. "**steamapps\common\World of Final Fantasy**" for Steam).

### Steam Deck/Linux Additional Instructions
🚩**You do not need to do this if you are using Windows!**
- Open up the game properties in Steam and add `WINEDLLOVERRIDES="dinput8=n,b" %command%` to the launch options.

## Configuration
- See **WOFFFix.ini** to adjust settings for the fix.

## Known Issues
Please report any issues you see.
This list will contain bugs which may or may not be fixed.

## Screenshots

| ![ezgif-2-f4932dc17d](https://github.com/Lyall/WOFFFix/assets/695941/5716a99d-db4f-4079-8246-ee51dda4df26) |
|:--:|
| Gameplay |

## Credits
[Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) for ASI loading. <br />
[inipp](https://github.com/mcmtroffaes/inipp) for ini reading. <br />
[spdlog](https://github.com/gabime/spdlog) for logging. <br />
[safetyhook](https://github.com/cursey/safetyhook) for hooking.
