import serial
import time
timeout = 1.0

ser = serial.Serial('COM9', 115200)
while True:
    timer = time.time()
    while time.time() < timer + timeout:
        while ser.in_waiting > 0:
            print (ser.read().decode('UTF-8'), end='')
    
    s = input("Zadaj konfiguračný reťazec [I|O|IO][A][B] [G<1..15>] ")
    command = 0
    if s.find("O") != -1:
        command += 128
    if s.find("I") != -1:
        command += 64

    if s.find("A") != -1:
        command += 32

    if s.find("B") != -1:
         command += 16

    ig = s.find("G")
    gates = 0
    if ig >= 0:
      if ig + 1 < len(s):
          c = ord(s[ig + 1]) - ord('0')
          if (c >= 0) and (c <= 9):
              gates = c
              if ig + 2 < len(s):
                  c = ord(s[ig + 2]) - ord('0')
                  if (c >= 0) and (c <= 9):
                      gates *= 10
                      gates += c
                      
    if ((gates < 1) or (gates > 15)):
        gates = 0
    command += gates

                      
    listx = [0x4E, command, 255 - command, 0xFE]
    ser.write(bytes(listx))

    while True:
        while ser.in_waiting > 0:
            print (ser.read().decode('UTF-8'), end='')
