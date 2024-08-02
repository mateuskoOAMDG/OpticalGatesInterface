#Optical gate interface driver v.0.3.001
'''
Driver for Optical Gate Interface v.1.1

The Driver collects data from the Interface
and writes it to the data.csv file, which is in the same directory as this driver.

It automatically finds the Serial (e.g. COMx) port to which the interface is connected.

It Accepts commands for Interface now

Added support for gates-count measure.

(c) mateusko O.A.M.D.G 2024-07-29
'''
from operator import attrgetter

import serial
import serial.tools.list_ports
import os
import time
import sys

#Interface recognition texe
pass_text = "#Optical_Gate_Interface v.1"

#Output filename
filename = "data.csv"
file = False

# Mode setting
buffered = True
autostart = True
onIn = True
onOut = False
capture = False
data_num = 0

class ArduinoSerial:
    serial = serial.Serial()

    @staticmethod
    def get_ports_with_description():
        arrtuple = serial.tools.list_ports.comports()
        return arrtuple
        for item in arrtuple:
            (port, description, sysinfo) = item
            print(f'{port}: {description} - ({sysinfo}) ')

    def open(self, port, baudrate=115200, timeout=0.5):
        self.serial.port = port
        self.serial.baudrate = baudrate
        self.serial.timeout = timeout
        self.serial.open()

    def readline(self):
        """Prečíta riadok zo Serial, lebo vráti False, ak nie sú dátak dispozícii"""
        if (self.serial.in_waiting > 0):
            ans = self.serial.readline()
            ans = ans.decode("ascii", "replace")
            return ans
        return False

    def clear_buffer(self):
        """Vyprázdni vstupný buffer"""
        self.serial.reset_input_buffer()
        while self.serial.in_waiting > 0:
            self.serial.readall()

    def is_connected(self):
        return self.serial.isOpen()

    def close(self):
        self.serial.close()

ser = ArduinoSerial()
def init():
    print('+-----------------------------------+')
    print('|   Optical Gate Interface Driver   |')
    print('|          version: 0.0.3           |')
    print('|                                   |')
    print('| (c) 2023-24, mateusko O.A.M.D.G   |')
    print('+-----------------------------------+')
def scan_ports():
    return ser.get_ports_with_description()
# pokus o otvorenie portu "port" nic viac; vracia True/False - Uspech/Neuspech
def attempt_to_open_port(port):
    ''' Pokus o otvorenie portu "port"
        Vracia True / False = Úspešne otvorený / Neotvorený'''
    try:
        ser.close()
    finally:
        pass
    try:

        ser.open(port)
        return True
    except:
        return False
def attempt_connect(port, timeout = 1):
    '''
    Pokúsi sa otvoriť port s interfejsom "#Optical_Gate_Interface v.1"
    Caka na prijatie pass_text max. timeout sec.
    '''
    try:
        ser.close()
    finally:
        pass

    try:
        ser.open(port)
    except:
        return False

    t = time.perf_counter()
    ok = False

    # caka cas timeout, ci port vrati nejake data
    while (time.perf_counter() - t < timeout): #wait 1sec for answer
        line = ser.readline()
        if line != False:
            ok = True
            break

    if ok:
        line = line.strip()
        if line != pass_text:
            #neidentifikovany interfejs (neposlal pass_text)
            try:
                ser.close()
            finally:
                pass
            ok = False
    else:
        # Timeout! Not Connected
        ok = False

    return ok
def scan_ports_to_connection():
    '''
    Function scan all ports and wait for ack: #Optical_Gate_Interface
    Returns port name/False - Succesfully connected Yes/No
    '''

    #get port list
    ports = []
    for port, desc, info in scan_ports():
        ports.append(port)

    #print list
    port_conn = False
    ok = False
    for port in ports:
        print(f"Hľadám interface na porte {port} ... ")
        result = attempt_connect(port)
        if result:
            print(f"Pripojené! Interface počúva na porte {port}.")
            port_conn = port
            ok = True
            break

    if not ok:
        print("Interface sa nenašiel!")
        return False
    else:
        return port_conn
def close_serial():
    try:
        ser.close()
        return True
    except:
        return False
def test_open_port():
    ''' Ručné otvorenie portu. Vypíše sa zoznam portov a ručne sa zadá v konzole číslo portu'''
    while(True):
        for port, desc, info in scan_ports():
            print(f'{port} -  {desc} ')
        port = input("Zadaj číslo portu (bez COM)>")
        if port.strip() == "c":
            result = ser.close()
            print(f'Výsledok zatvorenia: {result}')
            print("Port zatvorený")
            continue
        port = "COM" + port


        print(f"Otváram port {port} ...")
        if attempt_to_open_port(port):
            print("Úspešné pripojenie")
        else:
            print("Nepodarilo sa pripojiť")
def printToFile(line):
    global file, buffered, data_num
    print(line)
    try:
        if not buffered:
            data_num += 1
            file.write(f'{data_num};')
        file.write(f'{line}\n')
    except:
        return False

    return True
def openFile():
    global filename, file, onIn, onOut
    opened = False
    try:
        file.close()
    except:
        pass
    finally:
        file = False

    try:
        file = open(filename, "w")
        file.write("N;")
        if onIn and onOut:
            if buffered:
                file.write("In;Out\n")
            else:
                file.write("In or Out\n")
        elif onIn:
            file.write("In\n")
        elif onOut:
            file.write("Out\n")
        opened = True
    except:
        opened = False
    return opened
def closeFile():
    global file
    opened = False
    if file != False:
        file.close()
    print("> File Closed")
    return True
def main2(port):
    global buffered, autostart, onIn, onOut, data_num
    capture = False
    attempt_no = 0
    frst_attmpt_msg = "Pripojte interfejs a stlačte Enter (Exit = q)"
    next_attmpt_msg = "Pripojte interfejs a stlačte Enter\nAk je interface pripojený do USB, odpojte ho, a znova pripojte. (Exit = q)"
    while (True):
        if not port:
            attempt_no += 1
            if attempt_no > 1:
                msg = next_attmpt_msg
            else:
                msg = frst_attmpt_msg

            if input(msg) == "q":

                close_serial()
                exit(0)
            port = scan_ports_to_connection()
            continue
        else:
            attempt_no = 0
        capture = False #Capturing to file not active
        print("Prijaté dáta (časy v mikrosekundách):")

        sendCommand(readArg())
        try:
            while (True):
                # capture data
                line = ser.readline()
                if line == False:
                    continue #waiting for data - infinite loop

                line = line.strip()
                if len(line) == 0:
                    continue #waiting for data - infinite loop

                if line.find("#START#") == 0:
                    if openFile():
                        print(f'> File {filename} opened')
                        capture = True
                        print("> Start capture")
                        data_num = 0
                    else:
                        print(f'> Error: file {filename} could not be opened')
                        capture = False



                elif line.find("#STOP#") == 0:
                    capture = False
                    closeFile()
                    print("> End of capture")

                elif capture:
                    if not printToFile(line):
                        print(f"> Write to file {filename} error")
                        capture = False
                else:
                    if line.find("B+") > 0:
                        buffered = True
                        print("> SET MODE: Citanie do buffra zapnute")
                    elif line.find("B-") > 0:
                        buffered = False
                        print("> SET MODE: Citanie do buffra vypnute, online zaznam dat")

                    if line.find("A+") > 0:
                        autostart = True
                        print("> SET MODE: Autostart zapnuty")
                    elif line.find("A-") > 0:
                        print("> SET MODE: Autostart vypnuty")
                        autostart = False


                    if line.find("IO") > 0:
                        onIn = True
                        onOut = True
                        print("> SET MODE: Zaznam v okamihu vstupu aj vystupu")
                    elif  line.find("I") > 0:
                        onIn = True
                        onOut = False
                        print("> SET MODE: Zaznam v okamihu vstupu")
                    elif line.find("O") > 0:
                        onIn = False
                        onOut = True
                        print("> SET MODE: Zaznam v okamihu vystupu")

        except serial.SerialException as err:
            print(f"Chyba komunikácie s interfejsom!\n Error: {err}")

            result = attempt_connect(port)
            if not result:
                port = scan_ports_to_connection()

        except Exception as err:
            print(f"Error: {err}, ")
            exit(1)

def readArg():
    arg =sys.argv
    argstring = "IAB"
    if len(arg) == 2:
        argstring = arg[1].strip()
    if len(argstring) == 0:
        argstring = "IAB"
    return argstring   

def sendCommand(s):
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
    ser.serial.write(bytes(listx))    

# ----------- start --------
init()


port = scan_ports_to_connection()
main2(port)


