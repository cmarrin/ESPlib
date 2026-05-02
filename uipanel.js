        console.log("uipanelJSON=" + uipanelJSON);

        // Widget is an object with type, name and label properties.
        // Return a string with the HTML for the widget
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
            fetch(jsonPath, {mode: 'no-cors'})
                .then(response => response.text())
                .then(data => {
                    const items = JSON.parse(data);

                    document.getElementById('title').innerText = items.title ? items.title : "Unknown Title";

                    for (const widget of items.widgets) {
                        const htmlString = makeWidget(items.name, widget);
                        console.log("htmlString='" + htmlString + "'");
                        document.getElementById('uipanel').innerHTML += htmlString;
                    }
                    for (const widget of items.widgets) {
                        addWidgetListener(items.name, widget);
                    }
                })
                .catch(error => console.error('Error fetching UI Panel', error));
        }
        
        function sendWidgetChange(name, widget, value)
        {
            const uri = `/uipanel/?name=${name}&widget=${widget}&value=${encodeURIComponent(value)}`;
            const http = new XMLHttpRequest();
            http.open("GET", uri, true);
            http.onreadystatechange = function()
            {
                if (http.readyState == 4) {
                    if (http.status != 200) {
                        alert(http.responseText);
                    }
                }
            }
            http.send();
        }

        makeUIPanel(uipanelJSON);
