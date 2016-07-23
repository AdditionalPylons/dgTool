1. Put dgTool.nds in the root of your 3ds sdmc card.
2. Place the dgTool folder with it's files inside in the root as well.
3. Run the app with the dsiware entry point of your choice. Flashcards will not work - no access to NAND.
4. Dump your 3ds firm0 firm1. Make sure OLD/NEW 3DS at the top matches your model of 3ds. Only 3ds's with "New" in the title are NEW 3DS.
5. Select "Downgrade to 10.4" to downgrade your 11.0 firm to 10.4, allowing kernel hax.
6. Run any kernel hax as a 3dsx in 3ds mode to test downgrade success.

This app only writes to FIRM0, not FIRM1, so it should be safe given your FIRM1 is not corrupt.
Never use this on anything but 3ds firm 11.0.X-YZ. Never use this if arm9loaderhax is installed; on any firmware.
Remember, if something goes wrong, it's your fault! NAND writing is always a risk. Read the LICENSE.txt (MIT) included for details.

Based on Wintermute's fwTool.
https://github.com/WinterMute/nintendo-ds-tools/tree/master/firmware/nds/fwTool