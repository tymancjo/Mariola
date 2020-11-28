# This is file intended for development of the Dz3 work with tensorflow
# 
# TODO: test od shell command execution from python - DONE 
# TODO: figureout best way to put Dz3 control into library like structure
# TODO: move PID controls to OOP solution - shall be more clean in code

import os
import subprocess
import bluepy.btle_old as btle
from time import sleep
import math



class ReadDelegate(btle.DefaultDelegate):
    def handleNotification(self, cHandle, data):
        print(data.decode("utf-8"))
 


p = btle.Peripheral("88:25:83:f1:05:a1")
# p.setMTU(200) # byte size you need
s = p.getServiceByUUID("0000ffe0-0000-1000-8000-00805f9b34fb")
D3 = s.getCharacteristics()[0]




try:

    while True:
        command = input(" Command: ")
        
        if command.lower() in ["quit", "exit", "off"]:
            print("Exiting...")
            break

        c = command.split(",")
        c = [int(x) for x in c]
        
        # calculating the wheels
        distance = c[0]
        dir = c[1]

        sinus = math.sin(math.degrees(dir))
        cosinus = math.cos(math.degrees(dir))

        direction_vector = [[1,1,1,1], # 1 up
                        [-1,1,1,-1], # left
                        [-1,-1,-1,-1], # down
                        [1,-1,-1,1], # right
                        [0,1,1,0], # 45
                        [-1,0,0,-1], # 135
                        [0,-1,-1,0], # 225
                        [1,0,0,1], # 315
                        
                        ]   
        
        w = [];
        for d in direction_vector[dir-1]:
            w.append(int(d * distance))
        
        command = f"<1,{w[0]},{w[1]},{w[2]},{w[3]},0>"

        print(bytes(command, "utf-8"))
        dlugosc = (len(bytes(command, "utf-8")))

        if dlugosc > 19:

            commands = []
            commands.append(f"<1,{w[0]},")
            commands.append(f"{w[1]},{w[2]},")
            commands.append(f"{w[3]},0>")

            for com in commands:
                try:
                    # D3.write(bytes(command, "utf-8"))
                    D3.write(com.encode("utf-8"))
                    sleep(0.1)
                    pass
                except:
                    print("kiszka")
                    pass
                
        else:

            try:
                # D3.write(bytes(command, "utf-8"))
                D3.write(command.encode("utf-8"))
                pass
            except:
                print("kiszka")
                pass
        
        sleep(0.3)

        p.withDelegate(ReadDelegate())
        # while True:
        while p.waitForNotifications(0.3):
            pass


        
except KeyboardInterrupt:

    print("Press Ctrl-C to terminate while statement")
    p.disconnect()
    pass

p.disconnect()

