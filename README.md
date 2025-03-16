# OBS Hadowplay
![Logo](/obs-hadowplay-icon.png)
## Introduction

In an attempt to move away from ShadowPlay and it's lack of customization, I thought I'd bring the useful functionality over to OBS.

This includes automatic replay buffer start/stop based on hooked captures within the active scene and the organisation of replays/recordings into folders named after the capture subject.

## Changelog
* v2.0.5
  * Now works with all types of window-capture and game-capture
  * Improved capture subject name retrieval
    * No longer relies on enumerating fullscreen apps.
    * Grabs the top active capture's Product Description, Product Name or exe name at the time of saving.
    * Exceptions now only need to target applications that can be captured.
  * Added user settings under the Tools menu
    * Replay buffer settings
      * Automatic replay buffer can be toggled off
      * Configure automatic stop delay in seconds
      * Toggle replay buffer restart on saving
    * Organisation settings
      * Automatic file organisation can be toggled off
      * Toggle organisation of screenshots
      * Toggle using the discovered process name as the file prefix
    * Settings are saved at the scene collection level
  * No longer starts recording if all running fullscreen windows are excluded
    * User exclusions can be added through the settings menu
  * Added notifications when recordings are saved
    * Both sound and dekstop notifications can be toggled through the settings menu
  * Automatic replay now checks inside groups and scene references for game-captures

* v1.1.3
  * Added automatic replay organisation
    * When a reply is saved, it will move the recording into a folder named after the top most application's ProductName
  * Added automatic recording organisation


* v1.0.0
  * Initial release.
    * Automatic activation of the replay buffer when a game-capture is detected as being active (width & height > 0)
    * Manual replay buffer activation will not be stopped if a game-capture becomes inactive
    * Manual replay buffer deactivation is possible and will restart once game-captures become inactive

Currently only works on Windows

Language packs
* English
* Russian
* French
* Chinese
* Taiwanese