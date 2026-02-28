--
-- testLED - test the operation of the led_strip API
--

initLED(1, 10, 8)

local down = false

for j = 1, 10, 1 do
	for i = 0, 50, 1 do
		local brightness = down and (50 - i) or i
		setLED(1, 0, 0, 0, brightness)
		setLED(1, 1, 0, brightness, 0)
		setLED(1, 2, 0, brightness, brightness)
		setLED(1, 3, brightness, 0, 0)
		setLED(1, 4, brightness, 0, brightness)
		setLED(1, 5, brightness, brightness, 0)
		setLED(1, 6, brightness, brightness, brightness)
		setLED(1, 7, brightness, 0, 0)
		refreshLEDs(1)
		delay(40)
	end
	print("*** loop "..j)
	down = not down
end

