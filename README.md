# uhd-helper
## What is this
IF you own more than one USRP variable, you'll probably notice that all of them has different bitstreams and fx3 firmwares. While it is painful if you trying to use them from one to another.  
This tool manages your bitstreams by "profiles" and you can control them by group. Create groups, load specific groups or delete specific groups, in minimal step.

## System Requirment
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
please make sure you have original un-touch UHD dir before your first boot, because i cannot identify if you are having original un-touched UHD, so I have to treat the one as official on first boot.

### basic opration
- Profiles panel let you pick an profile and either active it or delete it.
- the actions panel is context menu of profiles panel.
- the buttons panel let you do basic oprations.
