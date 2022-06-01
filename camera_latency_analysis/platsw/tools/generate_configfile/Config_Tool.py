import tkinter
from tkinter import *
from tkinter import ttk, messagebox, font
import json
import hashlib

tool_version = "V1.1"
#     - 2: XCU2.0 (Tahoe1.1)
#     - 3: CaSH1.3
board_types_str = [
    'Tahoe1.1',
    'Cash1.3'
]
board_types = {
    'Tahoe1.1': '2',
    'Cash1.3': '3',
}
camera_pixel_sizes = ['8mp']
camera_format_types = ['8*RGB', '16*YUYV', '16*UYUV']
video_formats = {
    "8*RGB": '3',
    "16*YUYV": '4',
    "16*UYUV": '7'
}
delaymap_config_types = {
        "conf1:US-Pacifica 001 with Leopard Image AR0233 Camera":
        ["42 42 60 67 80 04 17 24", "42 42 42 67 92 17 45 90"],  # board1 and board2

        "conf2:14 AR0820 cameras, lidar rotating clockwise":
        ["97 03 15 28 40 60 72 85", "97 03 0 25 50 75 25 75"],  # board1 and board2

        "conf3:14 AR0820 cameras, left lidar rotating clockwise, right lidar counterclockwise":
        ["11 11 96 83 68 68 83 96", "08 08 0 25 50 75 25 75"],

        "conf4:28 AR0820 cameras, lidar rotating clockwise":
        ["97 03 12 25 40 60 75 88 40 60 69 69 97 03 32 32", "0 0 69 69 32 32 0 0 50 50 12 25 75 88 50 50"],

        "conf5:14 AR0820 cameras, right lidar rotating clockwise, left lidar counterclockwise":
        ["53 53 68 81 96 96 83 68", "56 56 0 75 50 75 35 75"],

        "conf6:14 X8B cameras, right lidar rotating clockwise, left lidar counterclockwise":
        ["90 90 02 15 27 27 15 02", "90 90 26 01 76 01 45 90"]
    }

default_cfg = '''
{
    "_comment"     : "tahoe1, virtual channel, lidar rotate clockwise and clockwise",
    "platform"     : "2",
    "cameraModels" : "4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4",
    "runMode"      : "2 2 2 2 2 2 2 2 2 2 2 2 2 2 2 2",
    "delayms"      : "97 03 12 25 40 60 75 88 40 60 69 69 97 03 32 32",
    "nightMode"    : "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0",
    "exposure"     : "0x40 0x40 0x40 0x40 0x40 0x40 0x40 0x40 0x40 0x40 0x40 0x40 0x40 0x40 0x40 0x40",
    "testMode"      : "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0",
    "tpgPattern"    : "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0",
    "inVideoFormat" : "1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1",
    "outVideoFormat": "4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4",
    "inputFps"      : "30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30",
    "outputFps"     : "10 10 10 10 10 10 10 10 10 10 10 10 10 10 10 10",
    "frameWidth"    : "3840 3840 3840 3840 3840 3840 3840 3840 3840 3840 3840 3840 3840 3840 3840 3840",
    "frameHeight"   : "2160 2160 2160 2160 2160 2160 2160 2160 2160 2160 2160 2160 2160 2160 2160 2160"
}'''

class Config_Tool() :
    def __init__(self):
        self.root = Tk()
        self.root.title("Config Generate Tool - " + tool_version)
        self.root.geometry('2620x600+10+10')
        self.row_index = 0
        self.config_json = json.loads(default_cfg)
        self.font_f1 = font.Font(family='microsoft yahei', size=12, weight='bold')
        self.font_f2 = font.Font(family='times', size=12)

        # Canvas,Scrollbar put in root window
        self.canvas = Canvas(master=self.root, width=2600, height=580, bd=2)
        self.scro = Scrollbar(master=self.root, orient=tkinter.HORIZONTAL)
        self.scro.pack(side='bottom', fill='x')
        self.canvas.pack(side='left')
        #create a frame to put other widgets
        self.window = Frame(self.canvas)
        self.window.pack()
        self.canvas.create_window((0, 0), window=self.window, anchor="nw")

    def create_base_widgets(self):
        Label(self.window, text="board_types:", width=14, font=self.font_f1).grid(row=self.row_index, column=0)
        self.combox_board_types = ttk.Combobox(self.window, value=board_types_str, width=14, font=self.font_f2)
        self.combox_board_types.current(0)  # default select 0
        self.combox_board_types.grid(row=self.row_index, column=1)
        self.combox_board_types.bind('<<ComboboxSelected>>', self.callback_boardtype_choose)

        Label(self.window, text="pixel_sizes:", width=14, font=self.font_f1).grid(row=self.row_index, column=2)
        self.combox_pixel_sizes = ttk.Combobox(self.window, value=camera_pixel_sizes, width=14, font=self.font_f2)
        self.combox_pixel_sizes.current(0)  # default select 0
        self.combox_pixel_sizes.grid(row=self.row_index, column=3)

        Label(self.window, text="format_types:", width=14, font=self.font_f1).grid(row=self.row_index, column=4)
        self.combox_format_types = ttk.Combobox(self.window, value=camera_format_types, width=14, font=self.font_f2)
        self.combox_format_types.current(0)  # default select 0
        self.combox_format_types.grid(row=self.row_index, column=5)
        self.combox_format_types.bind('<<ComboboxSelected>>', self.callback_format_type_choose)

        Label(self.window, text="board_no:", width=14, font=self.font_f1).grid(row=self.row_index, column=6)
        self.combox_board_numbers = ttk.Combobox(self.window, value=['0', '1'], width=14, font=self.font_f2)
        self.combox_board_numbers.current(0)  # default select 0
        self.combox_board_numbers.grid(row=self.row_index, column=7)

        self.row_index += 1
        Label(self.window, text="delay_configs:", width=14, font=self.font_f1).grid(row=self.row_index, column=0)
        self.delay_configs_name = []
        for temp in delaymap_config_types.keys():
            self.delay_configs_name.append(temp)
        temp_configs_name = [temp for temp in self.delay_configs_name]
        temp_configs_name.pop(3)
        self.combox_delaymap_configs = ttk.Combobox(self.window, value=temp_configs_name, width=70, font=self.font_f2)
        self.combox_delaymap_configs.current(0)  # default select 0
        self.combox_delaymap_configs.grid(row=self.row_index, column=1, columnspan=4)

        self.button_view = Button(self.window, text="View",
                                      borderwidth=3, fg='blue',
                                      command=self.callback_view, font=self.font_f1).grid(row=self.row_index, column=5)
        self.row_index += 1
        # label_list、entry_list、entry_variable、button_generate、modfiy_checkbt for delete later in create_second_widgets
        self.label_list = []
        self.entry_list = []
        self.entry_variable = []
        self.modify_checkbt_list = []
        self.modify_checkbt_variable = []
        self.button_generate = None

        # update frame
        self.window.update()
        self.canvas.configure(xscrollcommand=self.scro.set, scrollregion=self.canvas.bbox("all"))
        self.scro.config(command=self.canvas.xview)

    def create_second_widgets(self):
        self.key_list = []
        self.config_json = json.loads(default_cfg)

        # obtain the value from create_base_widgets
        select_board_type_str = self.combox_board_types.get().strip()
        select_board_type_num = board_types[select_board_type_str]

        select_format_type_str = self.combox_format_types.get().strip()
        select_format_type_num = video_formats[select_format_type_str]

        select_delaymap_config_str = self.combox_delaymap_configs.get().strip()
        select_board_number_int = int(self.combox_board_numbers.get().strip())
        select_delaymap_values_strs = delaymap_config_types[select_delaymap_config_str][select_board_number_int]

        comment_str = 'board_type={0}, board_no={1}, video_format={2}, delaymap_config= {3}'.format(select_board_type_str,
                       select_board_number_int, select_format_type_str, select_delaymap_config_str)

        self.config_json['_comment'] = comment_str
        self.config_json['platform'] = select_board_type_num
        self.config_json['delayms'] = select_delaymap_values_strs

        self.camera_nums_int = int(select_format_type_str.split('*')[0])
        outVideoFormat_str = ""
        for i in range(self.camera_nums_int):
            outVideoFormat_str += select_format_type_num
            outVideoFormat_str += " "
        self.config_json['outVideoFormat'] = outVideoFormat_str.strip()

        for key, value in self.config_json.items():
            tempLable = Label(self.window, text=key, width=14, font=self.font_f1)
            tempLable.grid(row=self.row_index, column=0)
            self.label_list.append(tempLable)
            if key == '_comment':
                temp_variable = StringVar()
                temp_variable.set(value)
                temp_entry = Entry(self.window, textvariable=temp_variable, width=140, font=self.font_f2)
                temp_entry.grid(row=self.row_index, column=1, columnspan=7)
                self.entry_list.append(temp_entry)
            elif key == 'platform':
                temp_variable = StringVar()
                temp_variable.set(value)
                temp_entry = Entry(self.window, textvariable=temp_variable, width=14, state='readonly', font=self.font_f2)
                temp_entry.grid(row=self.row_index, column=1)
                self.entry_list.append(temp_entry)

                self.row_index += 1 # add a row for index comment
                tempLable = Label(self.window, text="Modify", width=7, font=self.font_f1)
                tempLable.grid(row=self.row_index, column=1)
                self.label_list.append(tempLable)
                for i in range(self.camera_nums_int):
                    tempLable = Label(self.window, text="mipi{0}".format(i), width=14, font=self.font_f1)
                    tempLable.grid(row=self.row_index, column=i+2)
                    self.label_list.append(tempLable)
            else :
                #for modify check button
                temp_variable = IntVar()
                temp_variable.set(0)
                temp_checkBt = Checkbutton(self.window, variable=temp_variable, width=14, command=self.callback_checkbt)
                temp_checkBt.grid(row=self.row_index, column=1)
                self.modify_checkbt_list.append(temp_checkBt)
                self.modify_checkbt_variable.append(temp_variable)

                value_str_list = value.split(' ')
                temp_variable = [StringVar() for i in range(self.camera_nums_int)]
                for index, value_item in enumerate(value_str_list):
                    if index >= self.camera_nums_int:
                        break
                    temp_variable[index].set(value_item)
                    temp_entry = Entry(self.window, textvariable=temp_variable[index], width=14, state='readonly', font=self.font_f2)
                    temp_entry.grid(row=self.row_index, column=index+2)
                    self.entry_list.append(temp_entry)
            self.entry_variable.append(temp_variable)
            self.key_list.append(key)

            self.row_index += 1

        self.button_generate = Button(self.window, text="Generate",
                                      borderwidth=3,fg='blue',
                                      command=self.callback_generate, font=self.font_f1)
        self.button_generate.grid(row=self.row_index, column=0)
        # update frame
        self.window.update()
        self.canvas.configure(xscrollcommand=self.scro.set, scrollregion=self.canvas.bbox("all"))
        self.scro.config(command=self.canvas.xview)

    def callback_checkbt(self):
        for index, temp in enumerate(self.modify_checkbt_variable):
            if temp.get() == 1:
                for cam_no in range(self.camera_nums_int):
                    self.entry_list[2 + index * self.camera_nums_int + cam_no].config(state = 'normal')
            else :
                for cam_no in range(self.camera_nums_int):
                    self.entry_list[2 + index * self.camera_nums_int + cam_no].config(state = 'readonly')

    def callback_boardtype_choose(self, event):
        choose_val = self.combox_board_types.get()
        if choose_val == 'Cash1.3':
            self.combox_board_numbers['value'] = ('0')
        elif choose_val == 'Tahoe1.1':
            self.combox_board_numbers['value'] = ('0', '1')
        self.combox_board_numbers.current(0)

    def callback_format_type_choose(self, event):
        choose_val = self.combox_format_types.get()
        temp_cfg_list = [temp for temp in self.delay_configs_name]
        if choose_val == '16*YUYV' or choose_val == '16*UYUV':
            self.combox_delaymap_configs['value'] = [temp_cfg_list[3]]
        elif choose_val == '8*RGB':
            temp_cfg_list.pop(3)
            self.combox_delaymap_configs['value'] = temp_cfg_list
        self.combox_delaymap_configs.current(0)

    def callback_view(self):
        #delete widgets
        for temp in self.label_list:
            temp.destroy()
        self.label_list.clear()

        for temp in self.entry_list:
            temp.destroy()
        self.entry_list.clear()
        self.entry_variable.clear()

        for temp in self.modify_checkbt_list:
            temp.destroy()
        self.modify_checkbt_list.clear()
        self.modify_checkbt_variable.clear()

        if self.button_generate is not None:
            self.button_generate.destroy()

        #create new widgets
        self.create_second_widgets()

    def callback_generate(self):
        temp_index = 0
        for key, value in self.config_json.items():
            if isinstance(self.entry_variable[temp_index],list):
                temp_str = ""
                for temp in self.entry_variable[temp_index]:
                    temp_str += temp.get()
                    temp_str += " "
                self.config_json[key] = temp_str.strip()
            else:
                self.config_json[key] = self.entry_variable[temp_index].get().strip()
            temp_index += 1

        temp_str = json.dumps(self.config_json, indent=4)
        print(temp_str)

        messagebox_res = messagebox.askokcancel(title='Generate a new config', message='Generate a new config and md5 file?')

        #create a new config file
        if messagebox_res == True:
            with open("cfg_fg.json", 'w') as new_fd:
                new_fd.write(temp_str)

            with open('cfg_fg.json', 'rb') as md5_fd:
                temp_data = md5_fd.read()
            md5hash = hashlib.md5(temp_data)
            md5val = md5hash.hexdigest()
            print(md5val)

            with open('md5sum.txt', 'w') as md5_fd:
                md5_fd.write(md5val)

    def run(self):
        self.create_base_widgets()
        self.window.mainloop()

# Press the green button in the gutter to run the script.
if __name__ == '__main__':
    obj = Config_Tool()
    obj.run()

