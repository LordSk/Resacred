# compile shaders
import glob
import os

vs_files = glob.glob("vs_*.sc")
fs_files = glob.glob("fs_*.sc")

def fileReadWhole(file_path):
    f = open("%s.bin.temp" % name, "rb")
    file_buff = f.read()
    f.close()
    return file_buff

for vsf in vs_files:
    file_name = os.path.basename(vsf)
    print(file_name)
    name = file_name.split(".")[0]
    temp_path = "%s.bin.temp" % name

    buff = bytearray()
    base_cmd = "shaderc -f %s -o %s --type v -i \".\" --bin2c %s" % (file_name, temp_path, name)
    
    os.system(base_cmd + "_glsl --platform linux")
    buff += fileReadWhole(temp_path) + b"\n"
    
    os.system(base_cmd + "_spv -p spirv --platform linux")
    buff += fileReadWhole(temp_path) + b"\n"

    os.system(base_cmd + "_dx9 -p vs_3_0 -O 3 --platform windows")
    buff += fileReadWhole(temp_path) + b"\n"

    os.system(base_cmd + "_dx11 -p vs_4_0 -O 3 --platform windows")
    buff += fileReadWhole(temp_path) + b"\n"

    os.system(base_cmd + "_mtl -p metal --platform ios")
    buff += fileReadWhole(temp_path) + b"\n"
    buff += ("extern const uint8_t* %s_pssl;\n" % name).encode()
    buff += ("extern const uint32_t %s_pssl_size;\n" % name).encode()

    os.remove(temp_path)

    f = open("%s.bin.h" % name, "wb")
    f.write(buff)
    f.close()

for fsf in fs_files:
    file_name = os.path.basename(fsf)
    print(file_name)
    name = file_name.split(".")[0]
    temp_path = "%s.bin.temp" % name

    buff = bytearray()
    base_cmd = "shaderc -f %s -o %s --type f -i \".\" --bin2c %s" % (file_name, temp_path, name)
    
    os.system(base_cmd + "_glsl --platform linux")
    buff += fileReadWhole(temp_path) + b"\n"
    
    os.system(base_cmd + "_spv -p spirv --platform linux")
    buff += fileReadWhole(temp_path) + b"\n"

    os.system(base_cmd + "_dx9 -p ps_3_0 -O 3 --platform windows")
    buff += fileReadWhole(temp_path) + b"\n"

    os.system(base_cmd + "_dx11 -p ps_4_0 -O 3 --platform windows")
    buff += fileReadWhole(temp_path) + b"\n"

    os.system(base_cmd + "_mtl -p metal --platform ios")
    buff += fileReadWhole(temp_path) + b"\n"
    buff += ("extern const uint8_t* %s_pssl;\n" % name).encode()
    buff += ("extern const uint32_t %s_pssl_size;\n" % name).encode()

    os.remove(temp_path)

    f = open("%s.bin.h" % name, "wb")
    f.write(buff)
    f.close()