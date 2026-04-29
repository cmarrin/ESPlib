        document.getElementById('powerInput').addEventListener('input', function() {
            const on = this.checked;
            console.log("power is " + (on ? "on" : "off"));
        });
        document.getElementById('colorInput').addEventListener('input', function() {
            const color = this.value;
            console.log("Color changed to " + color);
        });
        
        console.log("uipanelJSON=" + uipanelJSON);
