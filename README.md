# OBS Hadowplay

## Introduction

In an attempt to move away from ShadowPlay and it's lack of customization, I thought I'd bring the useful functionality over to OBS.

Currently this includes automatic replay buffer start/stop based on a game capture within the active scene.

I'd love to add the functionality of moving replays into folders named after the game-capture subject, but currently this seems to be an impossabiltiy without changes to obs and it's plugins.

## Changelog
* v1.2.0
  * Added user settings under the Tools menu
    * Automatic replay buffer can now be toggled off
    * Settings are saved at the scene collection level
  * No longer starts recording if all running fullscreen windows are excluded
    * User exclusions can be added through the settings menu
  * Added notifications when recordings are organised into folders
    * Both sound and dekstop notifications can be toggled through the settings menu

* v1.1.3
  * Added automatic recording organisation **(windows only)**

* v1.1.1

  * Added automatic replay organisation **(windows only)**
    * When a reply is saved, it will move the recording into a folder named after the top most application's ProductName


* v1.0.0

  * Initial release. **(All platforms)**
    * Automatic activation of the replay buffer when a game-capture is detected as being active (width & height > 0)
    * Manual replay buffer activation will not be stopped if a game-capture becomes inactive
    * Manual replay buffer deactivation is possible and will restart once game-captures become inactive

Currently only tested on Windows-x64
