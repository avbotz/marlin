# Wiimote Control of Marlin

### Installation

Download and compile the *wiiuse* library at https://github.com/rpavlik/wiiuse.

You may need to execute `sudo ln -s /usr/local/lib/libwiiuse.so /usr/lib/libwiiuse.so` to link the shared library properly.

Then `make bin/wiimote` and it should be all set up.

### Usage

Run the bash script to test the program with sim state. There should be another bash script to test on the sub as soon as we get around to that.

