# ESPlib Web UI Panel

Many uses of the ESP32 have the need for some kind of UI. The most common example is controlling light strips. The UI needs to be able to turn the lights on and off, select animation effects and speed, set colors, etc. ESPlib supports in the form of a *Web UI Panel*.

A Web UI Panel is a JSON file which have specifications for a set of widgets to control aspects of whatever functions the ESP is doing. You specify these widgets in the JSON file with any layout requirements and what to do when a widget interaction takes place. This is typically sending a HTTP request to the server which executes some Lua control to activate the hardware the widget is controlling.

## Overview

The panel consists of a JSON file specifying the widget layout and behavior and a some Lua code to handle the HTTP requests resulting from the interactions with the widgets. A browser makes a request to an endpoint of the form:

	http://<hostname>/uipanel?op=new&name=<panel name>
	
This request serves a small webpage with references to CSS and JS files which implement the UI. The JS loads the JSON and uses it to build a UI panel. These files are located in the filesystem at:

    /fs/sys/ui/uipanel.css
    /fs/sys/ui/uipanel.js
    /fs/sys/ui/<panel name>.json
    
> **Note**: Currently only one UI panel is supported and its name is hardcoded to "**uipanel**". In the future multiple panels may be supported, each with its own unique *panel name*.

The webpage is mostly generic but includes two JS global variables. The first is *uiPanelJSON*, the path used to fetch the JSON file. The second is *uipanelName*, the root name of the panel. It's used to notify the server which panel was used when an interaction takes place. The JSON specifies the widget types which are known to the JS. The widgets are added to the DOM inside a **\<div>** with the id *uipanel*.

When a widget interaction takes place a request is sent to:

	http://<hostname>/uipanel?op=change&<params>
	
Params sent are *name* (the **\<panel name>**), *widget* (the name of the widget being changed), and *value* (the value the widget is being set to). When received a Lua program located at:

	/sys/ui/<panel name>.lua
	
is executed passing the *widget* and *value* parameters as string arguments. This program then performs whatever operation is appropriate for those arguments.

In addition to executing the Lua program, the value of the change is stored in a file at:

	/fs/sys/ui/<panel name>Values.json
	
The file contains JSON property/value pairs where the property is the widget name.

When the UI Panel is created it fetches the JSON to build the UI and then it requests the currently stored widget values as a request of the form:

	http://<hostname>/uipanel?op=values&name=<panel name>

The previously stored <panel name>Values.json file is fetched and returned.



