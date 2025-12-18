# ----- PIN-Setup usb-otg-board -----
# LCD ST7789 240x240
# 'BATTERY', 'BAT_VOL', 'BOOST_EN', 'BUTTON_DOWN', 'BUTTON_DW', 'BUTTON_MENU', 'BUTTON_OK', 'BUTTON_UP', 'DEV_VBUS_EN', 'DISPLAY', 'HOST_VOL', 'LCD_BL', 'LCD_DC', 'LCD_EN', 'LCD_RST', 'LCD_SCLK', 'LCD_SDA', 'LED', 'LED_GREEN', 'LED_YELLOW', 'LIMIT_EN', 'OVER_CURRENT', 'RX', 'SD_D0', 'SD_D1', 'SD_D2', 'SD_D3', 'SD_SCK', 'TX', 'UART', 'USB_SEL', 'VOLTAGE_MONITOR',

import board
from adafruit_display_text import label
#from adafruit_st7789 import ST7789

MOUNT_POINT = "/sd"
ICON_PATH = "/image"  # put your BMP icons here: sd.bmp, wifi.bmp, bt.bmp, usb.bmp, info.bmp

# display layout
DISPLAY = board.DISPLAY
ICON_W = 24
ICON_H = 24
LEFT_MARGIN = 6
TOP_MARGIN = 6
LINE_SPACING = 2
MAX_VISIBLE = 6

# Buttons (use board-provided names)
BTN_OK = board.BUTTON_OK
BTN_UP = board.BUTTON_UP
BTN_DW = board.BUTTON_DW
BTN_MENU = board.BUTTON_MENU

# ----- Debounced Button Klasse -----
class DebouncedButton:
    def __init__(self, pin, pull=digitalio.Pull.UP, debounce_ms=40, hold_ms=800):
        self._dio = digitalio.DigitalInOut(pin)
        self._dio.direction = digitalio.Direction.INPUT
        self._dio.pull = pull

        self.debounce_ms = debounce_ms / 1000.0
        self.hold_s = hold_ms / 1000.0

        self._last_raw = self._dio.value
        self._last_change = time.monotonic()
        self._stable = self._last_raw
        self._pressed_time = None
        self._event = None  # 'fell' | 'rose' | None

    def update(self):
        now = time.monotonic()
        raw = self._dio.value  # bei Pull-UP: gedrückt == False

        if raw != self._last_raw:
            # Zustand wechselte — starte debounce-timer
            self._last_raw = raw
            self._last_change = now
            self._event = None
            return

        # Wenn stabil lange genug, aktualisiere stabilen Zustand
        if (now - self._last_change) >= self.debounce_ms:
            if self._stable != raw:
                prev = self._stable
                self._stable = raw
                if not raw:  # FALLING (bei Pull-UP gedrückt)
                    self._pressed_time = now
                    self._event = 'fell'
                else:  # RISING (losgelassen)
                    self._event = 'rose'
                    self._pressed_time = None
            else:
                self._event = None

    def fell(self):
        return self._event == 'fell'

    def rose(self):
        return self._event == 'rose'

    def is_pressed(self):
        # True wenn aktuell stabil gedrückt
        return not self._stable

    def held(self):
        # True wenn gehalten länger als hold_s
        if self._pressed_time is None:
            return False
        return (time.monotonic() - self._pressed_time) >= self.hold_s




