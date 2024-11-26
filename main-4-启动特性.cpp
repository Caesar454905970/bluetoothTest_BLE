#include <windows.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Devices.Bluetooth.h>
#include <winrt/Windows.Devices.Bluetooth.Advertisement.h>
#include <winrt/Windows.Devices.Bluetooth.GenericAttributeProfile.h>

#include <winrt/Windows.Storage.Streams.h>

#include <iostream>
#include <iomanip>
#include <sstream>

using namespace winrt;
using namespace Windows::Devices::Bluetooth;
using namespace Windows::Devices::Bluetooth::Advertisement;
using namespace Windows::Devices::Bluetooth::GenericAttributeProfile;
using namespace Windows::Foundation::Collections;
using namespace Windows::Storage::Streams;
std::atomic<bool> keep_running(true);  // 控制程序是否继续运行

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
// 处理特性值变化的回调
// 处理特性值变化的回调
void OnCharacteristicValueChanged(const GattCharacteristic& characteristic, const GattValueChangedEventArgs& args) {
    std::wcout << L"Notification received for characteristic UUID: " << GuidToString(characteristic.Uuid()) << std::endl;

    // 获取接收到的值
    IBuffer buffer = args.CharacteristicValue();

    // 获取 buffer 的数据指针
    const uint8_t* data = buffer.data();

    // 获取 buffer 的长度
    uint32_t length = buffer.Length();

    // 打印接收到的数据
    for (uint32_t i = 0; i < length; ++i) {
        std::wcout << L"Data byte: " << (int)data[i] << std::endl;
    }
}

// 启用特性通知
// 启用特性通知
void EnableNotifications(const GattCharacteristic& characteristic) {
    // 使用按位与操作判断是否支持 Notify 属性
    if ((characteristic.CharacteristicProperties() & GattCharacteristicProperties::Notify) == GattCharacteristicProperties::Notify) {
        // 订阅特性值变化事件
        characteristic.ValueChanged({ OnCharacteristicValueChanged });

        // 启用通知
        auto result = characteristic.WriteClientCharacteristicConfigurationDescriptorAsync(
                GattClientCharacteristicConfigurationDescriptorValue::Notify
        ).get();

        if (result == GattCommunicationStatus::Success) {
            std::wcout << L"Notifications enabled for characteristic UUID: " << GuidToString(characteristic.Uuid()) << std::endl;
        } else {
            std::wcout << L"Failed to enable notifications for characteristic UUID: " << GuidToString(characteristic.Uuid()) << std::endl;
        }
    } else {
        std::wcout << L"Characteristic UUID " << GuidToString(characteristic.Uuid()) << L" does not support notifications." << std::endl;
    }
}

// 打印特性
void PrintCharacteristics(const GattDeviceService& service) {
    auto result = service.GetCharacteristicsAsync().get();

    if (result.Status() == GattCommunicationStatus::Success) {

        // 获取 GATT 特性集合
        IVectorView<GattCharacteristic> characteristics = result.Characteristics();
        for (auto const& characteristic : characteristics) {
            /*所有特性的 UUID*/
            std::wcout << L"  - Characteristic UUID: " << GuidToString(characteristic.Uuid()) << std::endl;
            std::wcout << L"      Properties: ";

            /*打印每个特性的 属性（Properties）*/
            GattCharacteristicProperties properties = characteristic.CharacteristicProperties();

            if ((properties & GattCharacteristicProperties::Read) == GattCharacteristicProperties::Read) {
                std::wcout << L"Read ";
            }
            if ((properties & GattCharacteristicProperties::Write) == GattCharacteristicProperties::Write) {
                std::wcout << L"Write ";
            }
            if ((properties & GattCharacteristicProperties::Notify) == GattCharacteristicProperties::Notify) {
                std::wcout << L"Notify ";
            }
            if ((properties & GattCharacteristicProperties::Indicate) == GattCharacteristicProperties::Indicate) {
                std::wcout << L"Indicate ";
            }


            std::wcout << std::endl;

            // 启用通知
            EnableNotifications(characteristic);
        }
    } else {
        std::wcout << L"Failed to retrieve characteristics for service: " << GuidToString(service.Uuid()) << std::endl;
    }
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

            // 打印每个服务的特性
            PrintCharacteristics(service);
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
//        std::wcout << L"Device found: "
//                   << args.Advertisement().LocalName().c_str()<<std::endl;
//        // 格式化 MAC 地址
//        std::wcout << L"Device MAC Address: "
//                   << FormatBluetoothAddress(args.BluetoothAddress())
//                   << L", RSSI: "
//                   << args.RawSignalStrengthInDBm()
//                   << L" dBm" << std::endl;

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
