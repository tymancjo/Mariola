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
 


# p = btle.Peripheral("88:25:83:f1:05:a1")
p = btle.Peripheral("88:25:83:f0:fe:e6")
# p.setMTU(200) # byte size you need
s = p.getServiceByUUID("0000ffe0-0000-1000-8000-00805f9b34fb")
D3 = s.getCharacteristics()[0]




try:

    while True:
        print("Syntax: dist,angle,turn_deg")
        command = input(" Command : ")
        
        if command.lower() in ["quit", "exit", "off"]:
            print("Exiting...")
            break

        c = command.split(",")
        c = [int(x) for x in c]
        
        # calculating the wheels
        distance = c[0]
        angle = c[1]
        turns = c[2]

        Dx = distance * math.sin(math.radians(-angle))
        Dy = distance * math.cos(math.radians(-angle))

        w = math.radians(turns)
        # dims = 2*math.pi*math.sqrt(4**2+9**2) / 
        dims = 4.5+9 

        w4 = int((Dy + Dx - w * dims))
        w3 = int((Dy - Dx - w * dims))
        w2 = int((Dy - Dx + w * dims))
        w1 = int((Dy + Dx + w * dims))


        
        command = f"<1,{w1},{w2},{w3},{w4},0>"

        print(bytes(command, "utf-8"))
        dlugosc = (len(bytes(command, "utf-8")))

        if dlugosc > 19:

            commands = []
            commands.append(f"<1,{w1},")
            commands.append(f"{w2},{w3},")
            commands.append(f"{w4},0>")

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

