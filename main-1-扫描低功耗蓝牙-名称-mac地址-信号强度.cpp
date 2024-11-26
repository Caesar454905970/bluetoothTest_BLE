#include <windows.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Devices.Bluetooth.Advertisement.h>
#include <iostream>
#include <iomanip> // 用于格式化输出
#include <sstream>

using namespace winrt;
using namespace Windows::Devices::Bluetooth::Advertisement;

std::wstring FormatBluetoothAddress(uint64_t address) {
    std::wstringstream ss;
    ss << std::hex << std::setfill(L'0');
    for (int i = 5; i >= 0; i--) {
        ss << std::setw(2) << ((address >> (i * 8)) & 0xFF);
        if (i > 0) {
            ss << L":";
        }
    }
    return ss.str();
}

int main() {
    init_apartment(); // 初始化 WinRT 环境

    BluetoothLEAdvertisementWatcher watcher;
    watcher.ScanningMode(BluetoothLEScanningMode::Active);

    // 设置设备发现的回调
    watcher.Received([](BluetoothLEAdvertisementWatcher const&,
                        BluetoothLEAdvertisementReceivedEventArgs const& args) {


        // 获取设备的广播名称
        std::wcout << L"Device found: "
                   << args.Advertisement().LocalName().c_str()<<std::endl;
        // 格式化 MAC 地址
        std::wcout << L"Device MAC Address: "
                   << FormatBluetoothAddress(args.BluetoothAddress())
                   << L", RSSI: "
                   << args.RawSignalStrengthInDBm()
                   << L" dBm" << std::endl;
    });

    // 启动扫描
    watcher.Start();
    std::wcout << L"Scanning for BLE devices..." << std::endl;

    // 等待扫描一段时间
    Sleep(10000); // 扫描 10 秒
    watcher.Stop();
    std::wcout << L"Scan stopped." << std::endl;

    return 0;
}
