# v1.4.8
- Bump geode version to support jitless
- Add a new cert validation setting in case HTTPS doesn't work for some reason
# v1.4.7
- Previews now use CCRenderTexture to optimize performance on large amounts of objects. (New setting: Pre-Render Object Capacity & Pre-Render Full View)
- Replaced GDAuth with argon
- Removed ProfilePage hook due to stability issues
# v1.4.6
- Sapphire SDK support
# v1.4.5
- Fixed bug with comments showing another players icon sometimes
- Fixed bug with the game crashing if you press enter while in the menu or on a profile
- Fixed cases not showing properly
- Fixed bug with going to user profiles bugging out if you aren't authenticated
- Fixed other miscellaneous bugs
# v1.4.4 
- Fixed issue with opening and closing ProfilePage randomly causing crashes
- Don't allow opening Object Workshop in ProfilePage while in a level (not sure why you would do that...)
- Added links to credits for Zidnes
# v1.4.1
- Ported to Geode v4.0.0-beta.1 (2.207!)
- Added DashAuth authentication option
# v1.4.0
- Allowed clicking on users profiles in comments (will only work for players that authenticated with Object Workshop before)
- Added \n for descriptions and comments
- Changed clicking on usernames in comments to redirect to the users objects rather than profile
- Allowed symbols in object names
- Changed descriptions (mainly editing) to use RobTop's textarea (like comments)
- Fixed bug where selecting an object in the workshop will not allow you to select in the editor unless you re-enter
- Added warnings
# v1.3.1
- Fixed bug where going to a users profile while on another page will assume you want to go to that page
# v1.3.0
- Added user profiles (Thanks Rue!)
- Replaced Trending with Featured
- Revamped Object UI
- Added abbreviations for object items (Object Count and Download Count)
- Fixed touch prio issues when filtering with tags, not being signed in, etc
- Fixed tags with spaces (Like "Auto Build") not working when filtering
- Fixed bug where you cannot go to the next page of "My Objects"
- (if you have the mod and when doggo accepts my pr) Added support for Emojis in Comments
# v1.2.2
- Added a requirement for 1 tag to be added for uploading
- Prevent searching in My Objects and Favorites
- Fixed issue with Object Workshop button not centering in the Editor
- Fixed the "reset zoom" button to also reset positioning
- Fixed zooming to "act like zooming" (instead of scaling based on the center)
- (hopefully) Fixed issue with crash on clicking on the cancel button of a comment popup
# v1.2.1
- Fixed bug with attempting to render invalid icons crashing
# v1.2.0
- Improved on the UI for clicking on objects (Thanks Rue!) - It will be overhauled again next major update.
- Allowed updating an object rather then needing to delete and reupload
- Added a "reset zoom" button
- Added comments
- Added info button
- Added Custom Object Bypass
- Removed the "Enabled" setting as its useless
- Fixed bug searching with too many tags
# v1.1.3
- Changed Android32 to NOT use TextArea (geode needs to fix gd::string!) basically setting a description is now fixed with a few caviats
- Made downloading pending objects not send a request.
- Updated Editor Tab API dependency ver
- Made scroll layer cancelling touches less sensitive
- Fixed issue with downloading/favoriting objects sometimes causing a crash when you exit too quickly
# v1.1.2
- Added some safe guards
# v1.1.1
- Made typing in pages work
- Made pressing escape while in an object not close the menu, but instead "go back"
- Added a loading circle so people dont accidentally click on the object workshop button twice
- Fixed issue searching with spaces causing an error
# v1.1.0
- Added color preview to objects (no color triggers work :<)
- Moved uploading to My Objects
- Added Object Count and Download Count on the Object Item
- Redesigned object summary
- Increased the amount of objects per page to be 9
- Allowed moving and zooming in/out the object in the preview
- Fixed issue with scrolling for objects (on Android as well!)
- Fixed issue with objects selecting when they're not supposed to when uploading an object
- (hopefully) Fixed issue with setting the description crashing on Android 32
# v1.0.5 
- Fixed issue with pausing the editor causing misalignment
- Fixed the "token expiration" popup
# v1.0.4
- Showcased whether or not your object is pending
- Allowed sending a report reason (required)
# v1.0.3
- Fixed downloading objects crashing when you're not authenticated
- Fixed other problems
# v1.0.2
- Quickly revert a change with Editor Tab API
# v1.0.1
- Added more to the mod description
- Added refresh button
- Added an error if server is down
# v1.0.0
- Mod Release
