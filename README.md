# uhd-helper
## What is this
If you own more than one USRP variant, you'll probably notice that all of them have different bitstreams and FX3 firmwares. It can be painful if you're trying to use them from one to another.  
This tool manages your bitstreams by "profiles," and you can control them by group. Create groups, load specific groups, or delete specific groups in minimal steps.

## System Requirement
- Arch Linux

## Usage
### Build
```
git clone https://github.com/zxkmm/uhd-helper.git
git submodule update --init --recursive
cd uhd-helper
cmake .. -G Ninja
ninja
```

### first boot
Please make sure you have the original, untouched UHD dir before your first boot, because I cannot identify if you have the original, untouched UHD, so I have to treat that one as official on first boot.

### basic operation
- The Profiles panel lets you pick a profile and either activate it or delete it.
- The actions panel is the context menu of the profiles panel.
- The buttons panel lets you do basic operations.
