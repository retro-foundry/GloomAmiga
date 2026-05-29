        /\___/\   ___ ___ _____ ___  ___  ___ ___  _   _ _   _ ____  ___  _   _
   ____/  _ _  \_| _ \ __|_   _| _ \/ _ \| __/ _ \| | | | \ | |  _ \| _ \| | | |
  / __   / V \   \   / _|  | | |   / (_) | _| (_) | |_| |  \| | | | |   /| |_| |
 /_/  \_/  ^  \___|_|_\___| |_| |_|_\\___/|_| \___/ \___/|_|\_|____/|_|_\ \__, |
 \_\__/\/\___/\/\_________________________________________________________ |___/
        \  \____/  /~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         \________/

                 GLOOM  -  PC / DOS PORT
                 VERSION   : 1.0.1
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

       DOS and Windows native builds also read GLOOM.INI from the game
       directory before command-line options are applied:

         resolution=320x200
         viewport_size=160x100
         viewport_pixel_size=2x2

       resolution accepts the same WIDTHxHEIGHT values as --resolution.
       viewport_size enables an Amiga-style centered gameplay window.
       viewport_pixel_size accepts 1x1 or 2x2.

       Keyboard controls can be changed in the same file:

         player1_control=Keyboard
         player2_control=Joystick1
         p1_forward=W,Up
         p1_back=S,Down
         p1_strafe_left=A
         p1_strafe_right=D
         p1_turn_left=Left,Q
         p1_turn_right=Right,E
         p1_fire=Space,Left Ctrl,Right Ctrl

       Player two uses the same names with p2_ instead of p1_.
       On DOS, player1_control and player2_control replace the old
       menu control selector rows.

       DEFAULT DOS LOW MODE:

         GLOOMPC.EXE --classic

       WIDESCREEN RENDER MODE:

         GLOOMPC.EXE --widescreen

       EXPLICIT WINDOW / RENDER SIZE:

         GLOOMPC.EXE --resolution 320x200
         GLOOMPC.EXE --resolution 640x400

       The aliases --window-size and --boot-resolution are also accepted.
       Native builds can additionally use --render-scale 1 through 6.

       Windows native builds can use optional HD presentation art exported
       from the repo art layout:

         gloom_pc.exe --hd-art D:\art-HD-v1

       GLOOM.INI also accepts:

         hd_art_path=D:\art-HD-v1

       The original Amiga binary assets are still required; HD art only
       replaces sampled presentation pixels for walls, flats, sprites,
       projectiles, sparks, and the gun.

   ---------------------------------------------------------------------

     LINKS

       PATREON  : https://www.patreon.com/c/retrofoundry
       RELEASES : https://retro-foundry.github.io/

   ---------------------------------------------------------------------

         "If it maps, crunches, raycasts, or beeps, it belongs here."
