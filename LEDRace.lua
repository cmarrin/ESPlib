--
-- race a LED along the strip
--
-- This tests turning all LEDs on and then changing only 
-- one LED on each loop

local NUM_LEDS = 8
local leds = { n = NUM_LEDS * 3 }

-- send a to led strip
function refresh(a)
	for i = 0, NUM_LEDS - 1, 1 do
		local index = i * 3 + 1
		setLED(1, i, a[index], a[index + 1], a[index + 2])
	end
	refreshLEDs(1)
end

-- fade all elements of a by multiplying by m and dividing by 256. m is 0-255
function fadeAll(a, m)
	for i = 1, NUM_LEDS * 3, 1 do
		a[i] = a[i] * m / 256
	end
end

-- set all colors of a to passed rgb value
function setColorAll(a, r, g, b)
	for i = 0, NUM_LEDS - 1, 1 do
		setColor(a, i, r, g, b)
	end
end

-- set color of led i to rgb value
-- i is 0 to NUM_LEDS - 1, but a is 3 times that size, so do the math
function setColor(a, i, r, g, b)
	local index = i * 3 + 1
	a[index] = r
	a[index + 1] = g
	a[index + 2] = b
end

initLED(1, 10, NUM_LEDS)
setColorAll(leds, 0, 0, 5)
refresh(leds)

__print__(string.format("before time=%d\n", millis()))

for j = 1, 5, 1 do
	local totalTime = 0
	for i = 0, NUM_LEDS, 1 do
		local t = millis();
		if i ~= NUM_LEDS then
			setColor(leds, i, 0, 5, 0)
		end
		if i ~= 0 then
			setColor(leds, i - 1, 0, 0, 5)
		end
		refresh(leds)
		local totalTime = totalTime + (millis() - t)
		delay(30)
	end
	__print__(string.format("totalTime=%d\n", totalTime))
end
__print__(string.format("after time=%d\n", millis()))

delay(1000)
fadeAll(leds, 0)
refresh(leds)


