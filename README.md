# Gstreamer_mp4_media_player
**GStreamer 1.16.2
Linux 20.04.01 Ubuntu**

This is an mp4 video+audio player which will take input from uri a .mp4 file and decodes the file and display the video+audio.

For Launching give the below command on Linux Ubuntu terminal :
gcc exercise.c -o exercise `pkg-config --cflags --libs gstreamer-1.0`
Above command will create executable file and for launching the application command will be **./exercise uri** otherwise it will give error and exit from the application.
