# Moonchild

Lua VM :crescent_moon: for Arduino

## Install

```sh
npm install -g moonchild
```

## Blink led

```lua
while true do
  light_on()
  delay_ms(100)
  light_off()
  delay_ms(100)
end
```

## Deploy to Arduino (requires AVR toolchain)

```sh
moonchild build script.lua
make -C build deploy
```


