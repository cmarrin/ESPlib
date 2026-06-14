// The UI Panel is built from a JSON file. This file contains all the
// info needed to build a UI panel. At the top of the panel is the
// title followed by a select button with all types of UI available.
// For instance if this is a panel to do light effects the select
// button would have each effect available. The rest of the UI is
// all the widgets used for the selected option.
//
// Note: if there is only one option entry, then the select button
// is skipped and only the widgets are shown.

// The JSON file has the following format:
//
//  "title" - title of the panel
//  "name"  - The <panel name> returned with changes to identify this panel
//  "ui"    - Object with all the UI panel options. Values are:
//
//      "label" - Label appearing on the select button
//      "list"  - Array of objects defining each UI type. Values are:
//
//          "label"     - Label that appears in the Select button entry
//          "program"   - Lua program to be run for this UI type (without the .lua suffix)
//          "widgets"   - Array of names of widgets in this UI
//          "params"    - Array of widget names sent to server when a widget changes
//
//  "widgets"   - Object of widgets available. <property> is widget name used in "params" above.
//                <value> is an object with the following values:
//
//      "type"  - Widget type. One of:
//
//          "toggleButon"   - on/off button
//          "colorPicker"   - standard HTML color picker with a single color
//          "select"        - select button. 
//
//      "label" - Label that appears on the widget
//      "list"  - (only for "select") array of values in the select button

// When a UI widget is changed a response is sent to the server with the query
// parameters "op=change&name=<panel name>&args=<list of params>
//
// The list of params is sent as a comma separated list of values in the 
// "params" array order of the currently selected UI. For color values
// the hue, saturation and brightness are sent as 3 numeric values in sequence.

// Widget is an object with type, name and label properties.
// Return a string with the HTML for the widget

let _widgetValues = null;

function makeWidget(panelName, widget)
{
    switch(widget.type) {
        case "toggleButton":
            return `<div class="widget">
                <label class="widget-label" for="${widget.name}">${widget.label}</label>
                <label class="widget-control switch">
                    <input type="checkbox" id="${widget.name}" >
                    <span class="slider round"></span>
                </label>
            </div>`;
        case "colorPicker":
            return `<div class="widget">
                <label class="widget-label" for="${widget.name}">${widget.label}</label>
                <input class="widget-control" type="color" id="${widget.name}" value="#000">
            </div>`;
        case "select":
            let s = `<div class="widget">
                 <label class="widget-label" for="${widget.name}">${widget.label}</label>
                   <select class="widget-control select-widget" id="${widget.name}">
                        <option value="">Select option:</option>`;
            
            for (const entry of widget.list) {
                s += `<option value="${entry}">${entry}</option>`;
            }
            s += "</select></div>";
            return s;
        default:
            console.log("makeWidget: unrecognized type - " + widget.type);
            return "";
    }
}

function addWidgetListener(panelName, widget)
{
    switch(widget.type) {
        case "toggleButton":
            document.getElementById(widget.name).addEventListener('input', function() {
                sendWidgetChange(panelName, widget.name, this.checked);
            });
            break;
        case "colorPicker":
            document.getElementById(widget.name).addEventListener('input', function() {
                sendWidgetChange(panelName, widget.name, this.value);
            });
            break;
        case "select":
            document.getElementById(widget.name).addEventListener('change', function() {
                sendWidgetChange(panelName, widget.name, this.value);
            });
            break;
        default:
            console.log("addWidgetListener: unrecognized type - " + widget.type);
            break;
    }
}

function makeUIPanel(jsonPath)
{
    // Fetch the JSON that defines the panel
    fetch(jsonPath, {mode: 'no-cors'})
        .then(response => response.text())
        .then(data => {
            const items = JSON.parse(data);

            document.getElementById('title').innerText = items.title ? items.title : "Unknown Title";

            for (const widget of items.widgets) {
                const htmlString = makeWidget(items.name, widget);
                document.getElementById('uipanel').innerHTML += htmlString;
            }
            for (const widget of items.widgets) {
                addWidgetListener(items.name, widget);
            }
        })
        .catch(error => console.error('Error fetching UI Panel', error));
}

function updateUIPanel(jsonPath)
{
    // Fetch the JSON that has the widget values
    fetch(jsonPath, {mode: 'no-cors'})
        .then(response => response.text())
        .then(data => {
            _widgetValues = JSON.parse(data);
            
            // If the JSON has no currentEffect property, make one
            if (!_widgetValues.hasOwnProperty("currentEffect")) {
                _widgetValues.currentEffect = "None";
            }
            // If the JSON has no effects property, make one
            if (!_widgetValues.hasOwnProperty("effects")) {
                _widgetValues.effects = { };
            }
        })
        .catch(function() {
            // File does not exist. Make an initial version
            _widgetValues = { currentEffect : "None", effects : { } };
        })
        .finally(() => {
            // Now we have a valid _widgetValues object, set the local UI and send the
            // widget values to the server
            if (_widgetValues.currentEffect !== "None") {
                sendWidgetChange(uipanelName, "effect", _widgetValues.currentEffect);
                
                if (_widgetValues.effects.hasOwnProperty(_widgetValues.currentEffect)) {
                    const items = _widgetValues.effects[_widgetValues.currentEffect]
                    for (const key in items) {
                        let widget = document.getElementById(key);
                        if (widget.type == "checkbox") {
                            widget.checked = items[key] == "true";
                        } else {
                            widget.value = items[key];
                        }
                        sendWidgetChange(uipanelName, key, items[key]);
                    }
                }
            }
        });
}

function sendWidgetChange(name, widget, value)
{
    const uri = `/uipanel?op=change&name=${name}&widget=${widget}&value=${encodeURIComponent(value)}`;
    const http = new XMLHttpRequest();
    http.open("GET", uri, true);
    http.onreadystatechange = function()
    {
        if (http.readyState == 4) {
            if (http.status != 200) {
                alert("sendWidgetChange error: status=" + http.status + "response=" + http.responseText);
            }
        }
    }
    http.send();
    
    uploadJSON(name);
}

function uploadJSON(name)
{
    let xhr = new XMLHttpRequest();
    const uri = `/uipanel?op=widgetValues&name=${name}`;
    xhr.open("POST", uri);
    xhr.setRequestHeader("Content-Type", "application/json");
    xhr.send(JSON.stringify(_widgetValues));
}

makeUIPanel(uipanelJSON);
updateUIPanel(uipanelWidgetValues);
