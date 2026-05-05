        /\___/\   ___ ___ _____ ___  ___  ___ ___  _   _ _   _ ____  ___  _   _
   ____/  _ _  \_| _ \ __|_   _| _ \/ _ \| __/ _ \| | | | \ | |  _ \| _ \| | | |
  / __   / V \   \   / _|  | | |   / (_) | _| (_) | |_| |  \| | | | |   /| |_| |
 /_/  \_/  ^  \___|_|_\___| |_| |_|_\\___/|_| \___/ \___/|_|\_|____/|_|_\ \__, |
 \_\__/\/\___/\/\_________________________________________________________ |___/
        \  \____/  /~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         \________/

                 GLOOM  -  PC / DOS PORT
                 VERSION   : 1.0.0
                 RELEASE   : FIRST BINARY RELEASE
                 TARGET    : PC / DOS 386+ / VGA / SOUNDBLASTER
                 DISPLAY   : 320X200 INDEXED VGA ON DOS
                 SOURCE    : ORIGINAL AMIGA GLOOM CODE AND ASSETS

   ---------------------------------------------------------------------

     Fight through the original Gloom levels with the port grounded in
     the Amiga source, maps, textures, object data, palettes, and sounds.

     DOS builds are tuned for low-resolution indexed rendering, bounded
     Sound Blaster audio pumping, and 386/486-era frame pacing.

   ---------------------------------------------------------------------

     RUNNING

       DOS      : GLOOM.BAT
       WINDOWS  : gloom_pc.exe
       VERSION  : gloom_pc.exe --version

     SAVE / LOAD

       Binary and web builds use separate one-player and two-player
       autosaves. Binary builds write these files beside the executable:

         GLOOM1P.SAV
         GLOOM2P.SAV

       Web builds store the same autosave records in browser localStorage.

       The game autosaves after every 3 completed campaign levels. When a
       save is written, GAME SAVED appears on screen.

       The main menu shows LOAD ONE PLAYER AUTOSAVE and LOAD TWO PLAYER
       AUTOSAVE under the matching new-game options. Each load option is
       drawn dark until that autosave exists. The pause menu loads the
       autosave matching the current one-player or two-player game. Combat
       games are not autosaved.

       New games and level transitions show the original Amiga script
       text before the next level loads.

     RESOLUTION OPTIONS

       DEFAULT DOS LOW MODE:

         GLOOMPC.EXE --classic

       WIDESCREEN RENDER MODE:

         GLOOMPC.EXE --widescreen

       EXPLICIT WINDOW / RENDER SIZE:

         GLOOMPC.EXE --resolution 320x200
         GLOOMPC.EXE --resolution 640x400

       The aliases --window-size and --boot-resolution are also accepted.
       Native builds can additionally use --render-scale 1 through 6.

   ---------------------------------------------------------------------

     LINKS

       PATREON  : https://www.patreon.com/c/retrofoundry
       RELEASES : https://retro-foundry.github.io/

   ---------------------------------------------------------------------

         "If it maps, crunches, raycasts, or beeps, it belongs here."
