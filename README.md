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
On first boot, the tool does not auto-create a special profile. If you want to preserve your current `images` state, create a profile from it before switching.

### basic operation
- The Profiles panel lets you pick a profile and either activate it or delete it.
- The actions panel is the context menu of the profiles panel.
- The buttons panel lets you do basic operations.

### notes
I regret to tell you but i'm sorry that i had dropped the compatibilities of previous version, in `58c613798742ba117160d33871f50c515acad7f7`. so if you use new code, please delete all config files and bitstreams and start over.
