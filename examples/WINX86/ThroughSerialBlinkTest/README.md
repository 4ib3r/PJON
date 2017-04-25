In this example is possible to connect a Windows X86 compatible device with an Arduino compatible device using the `ThroughSerial` strategy. Visual Studio 2016 Express is used to compile the program on the Windows X86 machine, to do so open the `PJON\examples\WINX86\ThroughSerialBlinkTest\ThroughSerialBlinkTest.cpp` example.

Now is necessary to right click on the `cpp` files branch in the project structure and choose option to "add existing resource" and browse for `serial.cpp` in `PJON\interfaces\WINX86\Serial\serial.cpp`.

Now save the project, change the COM port to match your configuration.

The last step is to program the Arduino compatible device using the `PJON\examples\ARDUINO\ThroughSerialBlinkTest\Receiver.ino` [sketch](../../ARDUINO/local/ThroughSerialBlinkTest/Receiver.ino) and the Arduino IDE.

Executing the program on the Windows X86 machine you should see the Arduino compatible device blinking.
