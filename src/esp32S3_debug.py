#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ESP32 Arduino 串口监控工具
实时输出串口打印信息
"""

import serial
import serial.tools.list_ports
import time
import sys

def list_ports():
    """列出可用串口"""
    ports = serial.tools.list_ports.comports()
    if not ports:
        print("❌ 未找到串口设备")
        return []
    
    print("\n可用串口:")
    for i, port in enumerate(ports):
        print(f"{i+1}. {port.device} - {port.description}")
    
    return [port.device for port in ports]

def monitor_serial(port_name, baudrate=115200):
    """监控串口输出"""
    try:
        # 连接串口
        ser = serial.Serial(port_name, baudrate, timeout=1)
        print(f"✅ 已连接 {port_name} ({baudrate} baud)")
        print("📊 开始监控串口输出 (按 Ctrl+C 退出)...\n")
        
        # 清空缓冲区
        ser.reset_input_buffer()
        
        while True:
            # 读取串口数据
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        print(line)
                except:
                    pass
            
            time.sleep(0.01)
            
    except serial.SerialException as e:
        print(f"❌ 串口错误: {e}")
    except KeyboardInterrupt:
        print("\n👋 停止监控")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()

def main():
    """主函数"""
    print("🔍 ESP32 串口监控工具")
    
    # 列出串口
    ports = list_ports()
    if not ports:
        return
    
    # 选择串口
    try:
        if len(ports) == 1:
            # 只有一个串口，直接使用
            port_choice = ports[0]
            print(f"🎯 自动选择: {port_choice}")
        else:
            # 多个串口，让用户选择
            choice = input(f"请输入串口号 (1-{len(ports)}): ").strip()
            if choice.isdigit() and 1 <= int(choice) <= len(ports):
                port_choice = ports[int(choice) - 1]
            else:
                # 直接输入端口名
                if choice in ports:
                    port_choice = choice
                else:
                    print("❌ 无效选择")
                    return
        
        # 开始监控
        monitor_serial(port_choice)
        
    except KeyboardInterrupt:
        print("\n👋 用户退出")
    except Exception as e:
        print(f"❌ 错误: {e}")

if __name__ == "__main__":
    main()