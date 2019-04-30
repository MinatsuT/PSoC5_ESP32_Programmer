# ESP32 Programmer using PSoC5LP
This firmware makes your PSoC5LP Prototyping Kit (CY8CKIT-059) into an ESP32 programmer with a _reset-to-bootloader_ feature. When you flash a binary by esptools.py, target ESP32 will boot in bootloader mode with no additional external circuit.

## Pin connections
|**PSoC5LP**|       |**ESP32**|
------------|-------|----------
|     P12_0 |&larr; | IO1(TX) |
|     P12_1 |&rarr; | IO3(RX) |
|     P12_2 |&rarr; | EN      |
|     P12_3 |&rarr; | IO0     |
|       GND |&mdash;| GND     |

Plug the **micro USB** on PSoC5LP Prototyping Kit (not the TYPE-A on the KitProg) to your host PC.

------------------------------------------------------------------------------
# PSoC5LPを用いたESP32プログラマ
PSoC5LP Prototyping Kit (CY8CKIT-059)をESP32用のプログラマにするファームウェアです。
esptools.pyでフラッシュするときの、ブートローダモードへの自動リセットに対応しています。
PSoC5LP Prototyping Kit以外に、特別な外部回路は必要ありません。

## PSoC5LPとESP32の接続
|**PSoC5LP**|       |**ESP32**|
------------|-------|----------
|     P12_0 |&larr; | IO1(TX) |
|     P12_1 |&rarr; | IO3(RX) |
|     P12_2 |&rarr; | EN      |
|     P12_3 |&rarr; | IO0     |
|       GND |&mdash;| GND     |

PSoC5LP Prototyping Kitの**micro USB**（KitProg側のTYPE-Aコネクタではありません）をホストPCに接続して下さい。


