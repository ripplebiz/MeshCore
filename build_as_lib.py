from os.path import realpath

Import("env") # type: ignore
menv=env # type: ignore

src_filter = [
  '+<*.cpp>',
  '+<helpers/*.cpp>',
  '+<helpers/sensors>'
  '+<helpers/radiolib/*.cpp>',
  '+<helpers/ui/MomentaryButton.cpp>',
]

# add build and include dirs according to CPPDEFINES
for item in menv.get("CPPDEFINES", []):
 
    # STM32
    if isinstance(item, str) and item == "STM32_PLATFORM":
        menv.Append(CPPPATH=[realpath("src/helpers/stm32")])
        menv.Append(BUILD_FLAGS=["-I src/helpers/stm32"])
        src_filter.append("+<helpers/stm32/*>")
    
    # ESP32
    elif isinstance(item, str) and item == "ESP32":
        menv.Append(CPPPATH=[realpath("src/helpers/esp32")])
        menv.Append(BUILD_FLAGS=["-I src/helpers/esp32"])
        src_filter.append("+<helpers/esp32/*>")
    
    # NRF52
    elif isinstance(item, str) and item == "NRF52_PLATFORM":
        menv.Append(CPPPATH=[realpath("src/helpers/nrf52")])
        menv.Append(BUILD_FLAGS=["-I src/helpers/nrf52"])
        src_filter.append("+<helpers/nrf52/*>")

    # RP2040
    elif isinstance(item, str) and item == "RP2040_PLATFORM":
        menv.Append(CPPPATH=[realpath("src/helpers/rp2040")])
        menv.Append(BUILD_FLAGS=["-I src/helpers/rp2040"])
        src_filter.append("+<helpers/rp2040/*>")
    
    # VARIANTS HANDLING
    elif isinstance(item, tuple) and item[0] == "MC_VARIANT":
        variant_name = item[1]
        menv.Append(BUILD_FLAGS=[f"-I variants/{variant_name}"])
        src_filter.append(f"+<../variants/{variant_name}>")

menv.Replace(SRC_FILTER=src_filter)

#print (menv.Dump())
