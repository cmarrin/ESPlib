--
-- Animate LEDs back and forth while changing colors
--

local NUM_LEDS = 60
local leds = { n = NUM_LEDS * 3 }

-- H is 0-360, S and V are 0-255
-- Returned r, g, b are 0-255
function hsvToRGB(H, S, V)
	if H < 0 or H > 360 or S < 0 or S > 255 or V < 0 or V > 255 then
		return 0, 0, 0
	end
	
	local s = S / 255
	local v = V / 255
	local C = s * v
	local X = C * (1 - math.abs(((H / 60) % 2) - 1))
	local m = v - C
	
	local r;
	local g;
	local b;
	if H < 60 then
		r = C
		g = X
		b = 0
	elseif H < 120 then
		r = X
		g = C
		b = 0
	elseif H < 180 then
		r = 0
		g = C
		b = X
	elseif H < 240 then
		r = 0
		g = X
		b = C
	elseif H < 300 then
		r = X
		g = 0
		b = C
	else
		r = C
		g = 0
		b = X
	end
	
	return (r + m) * 255, (g + m) * 255, (b + m) * 255
end

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
	if m == 0 then
		for i = 1, NUM_LEDS * 3, 1 do
			-- just clear the values
			a[i] = 0
		end
		return
	end
	for i = 1, NUM_LEDS * 3, 1 do
		a[i] = a[i] * m / 256
	end
end

-- set all colors of a to passed rgb value
function setColorAllRGB(a, r, g, b)
	for i = 0, NUM_LEDS - 1, 1 do
		setColor(a, i, r, g, b)
	end
end

-- set all colors of a to passed rgb value
function setColorAllHSV(a, h, s, v)
	setColorAllRGB(hsvToRGB(h, s, v))
end

-- set color of led i to rgb value
-- i is 0 to NUM_LEDS - 1, but a is 3 times that size, so do the math
function setColorRGB(a, i, r, g, b)
	local index = i * 3 + 1
	a[index] = r
	a[index + 1] = g
	a[index + 2] = b
end

-- set color of led i to hsv value
-- i is 0 to NUM_LEDS - 1, but a is 3 times that size, so do the math
function setColorHSV(a, i, h, s, v)
	setColorRGB(a, i, hsvToRGB(h, s, v))
end

initLED(1, 11, NUM_LEDS)

fadeAll(leds, 0)

local hue = 0;
local down = false
local start
local term
local inc

for j = 0, 5, 1 do
	if down then
		start = 0
		term = NUM_LEDS - 1
		inc = 1
	else
		term = 0
		start = NUM_LEDS - 1
		inc = -1
	end
	
	down = not down
	
	for i = start, term, inc do
		setColorHSV(leds, i, hue, 255, 255);
		hue = hue + 1
		if hue > 255 then
			hue = 0
		end
		refresh(leds);
		fadeAll(leds, 250);
		delay(20);
	end
end

fadeAll(leds, 0)
refresh(leds)


