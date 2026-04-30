        console.log("uipanelJSON=" + uipanelJSON);

        // Widget is an object with type, name and label properties.
        // Return a string with the HTML for the widget
        function makeWidget(widget)
        {
            switch(widget.type) {
                case "toggleButton":
                    return `<div class="widget" id="${widget.name}">
                        <label class="widget-label" for="${widget.name}">${widget.label}</label>
                        <label class="widget-control switch">
                            <input type="checkbox" id="${widget.name}" >
                            <span class="slider round"></span>
                        </label>
                    </div>`;
                case "colorPicker":
                    return `<div class="widget" id="${widget.name}">
                        <label class="widget-label" for="${widget.name}">${widget.label}</label>
                        <input class="widget-control" type="color" id="${widget.name}" value="#000">
                    </div>`;
                default:
                    console.log("makeWidget: unrecognized type - " + widget.type);
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
                        const htmlString = makeWidget(widget);
                        console.log("htmlString='" + htmlString + "'");
                        document.getElementById('uipanel').innerHTML += htmlString;
                    }
                })
                .catch(error => console.error('Error fetching UI Panel', error));
        }

        makeUIPanel(uipanelJSON);
