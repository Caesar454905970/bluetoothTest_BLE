#include <windows.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Devices.Bluetooth.h>
#include <winrt/Windows.Devices.Bluetooth.Advertisement.h>
#include <winrt/Windows.Devices.Bluetooth.GenericAttributeProfile.h>


#include <iostream>
#include <iomanip>
#include <sstream>

using namespace winrt;
using namespace Windows::Devices::Bluetooth;
using namespace Windows::Devices::Bluetooth::Advertisement;
using namespace Windows::Devices::Bluetooth::GenericAttributeProfile;
using namespace Windows::Foundation::Collections;
// 格式化 MAC 地址
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
std::wstring GuidToString(const winrt::guid& g) {
    std::wostringstream oss;
    // 格式化 GUID 为标准的 8-4-4-4-12 字符串
    oss << std::hex << std::uppercase
        << g.Data1 << L"-" << g.Data2 << L"-" << g.Data3 << L"-"
        << static_cast<int>(g.Data4[0]) << static_cast<int>(g.Data4[1]) << L"-"
        << static_cast<int>(g.Data4[2]) << static_cast<int>(g.Data4[3])
        << static_cast<int>(g.Data4[4]) << static_cast<int>(g.Data4[5])
        << static_cast<int>(g.Data4[6]) << static_cast<int>(g.Data4[7]);
    return oss.str();
}
// 打印服务列表
void PrintGattServices(BluetoothLEDevice device) {
    std::wcout << L"Connected to device: " << device.Name().c_str() << std::endl;
    auto result = device.GetGattServicesAsync().get();

    if (result.Status() == GattCommunicationStatus::Success) {

        std::wcout << L"Success Status: " << (int)result.Status() << std::endl;
        // 获取 GATT 服务集合
        IVectorView<GattDeviceService> services = result.Services();

        // 打印服务数量
        std::wcout << L"Found " << services.Size() << L" services:" << std::endl;

        for (auto const& service : services) {
            // 使用 ToString() 将 GUID 转换为字符串
            std::wcout << L" - Service UUID: " << GuidToString(service.Uuid()) << std::endl;
        }
    } else {
        std::wcout << L"Failed to retrieve services. Status: " << (int)result.Status() << std::endl;
    }
}

int main() {
    init_apartment(); // 初始化 WinRT 环境

    BluetoothLEAdvertisementWatcher watcher;
    watcher.ScanningMode(BluetoothLEScanningMode::Active);

    // 设置设备发现的回调
    watcher.Received([](BluetoothLEAdvertisementWatcher const&,
                        BluetoothLEAdvertisementReceivedEventArgs const& args) {
        hstring deviceName = args.Advertisement().LocalName();
        auto macAddress = FormatBluetoothAddress(args.BluetoothAddress());

        // 打印设备信息
        // 获取设备的广播名称
        std::wcout << L"Device found: "
                   << args.Advertisement().LocalName().c_str()<<std::endl;
        // 格式化 MAC 地址
        std::wcout << L"Device MAC Address: "
                   << FormatBluetoothAddress(args.BluetoothAddress())
                   << L", RSSI: "
                   << args.RawSignalStrengthInDBm()
                   << L" dBm" << std::endl;

        // 如果是目标设备，连接设备
        if (deviceName == L"ECG-7") {
            std::wcout << L"Target device found, connecting..." << std::endl;

            try {
                // 连接到设备
                auto device = BluetoothLEDevice::FromBluetoothAddressAsync(args.BluetoothAddress()).get();
                PrintGattServices(device); // 打印服务列表
            } catch (const hresult_error& ex) {
                std::wcerr << L"Failed to connect to device. Error: " << ex.message().c_str() << std::endl;
            }
        }
    });

    // 启动扫描
    watcher.Start();
    std::wcout << L"Scanning for BLE devices..." << std::endl;

    // 等待扫描一段时间
    Sleep(20000); // 扫描 20 秒
    watcher.Stop();
    std::wcout << L"Scan stopped." << std::endl;

    return 0;
}
