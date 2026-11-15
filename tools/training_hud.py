"""Non-activating, click-through HUD for the specified training game PID only."""
from __future__ import annotations

import argparse
import ctypes
from ctypes import wintypes
import json
import math
import os
from pathlib import Path
import time
import tkinter as tk
from PIL import Image, ImageTk
from training_settings import load as load_settings, save as save_settings, DEFAULTS

from training_channel import ProcessWatch


class TrainingHUD:
    def __init__(self, pid: int, status_file: Path):
        if os.name != "nt":
            raise RuntimeError("The native-game HUD requires Windows")
        self.pid, self.status_file = pid, status_file
        self.watch = ProcessWatch(pid)
        self.user = ctypes.WinDLL("user32", use_last_error=True)
        try:
            self.user.SetProcessDpiAwarenessContext.argtypes = [ctypes.c_void_p]
            self.user.SetProcessDpiAwarenessContext(ctypes.c_void_p(-4))
        except (AttributeError, OSError):
            pass
        self.user.GetForegroundWindow.restype = wintypes.HWND
        self.user.GetWindowThreadProcessId.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)]
        self.user.GetClientRect.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.RECT)]
        self.user.ClientToScreen.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.POINT)]
        self.user.IsWindowVisible.argtypes = [wintypes.HWND]
        self.user.IsIconic.argtypes = [wintypes.HWND]
        self.user.GetWindowTextLengthW.argtypes = [wintypes.HWND]
        self.user.GetWindowTextW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
        self.user.GetAncestor.argtypes = [wintypes.HWND, wintypes.UINT]
        self.user.GetAncestor.restype = wintypes.HWND
        self.user.GetWindowLongPtrW.argtypes = [wintypes.HWND, ctypes.c_int]
        self.user.GetWindowLongPtrW.restype = ctypes.c_ssize_t
        self.user.SetWindowLongPtrW.argtypes = [wintypes.HWND, ctypes.c_int, ctypes.c_ssize_t]
        self.user.SetWindowLongPtrW.restype = ctypes.c_ssize_t
        self.user.SetWindowPos.argtypes = [wintypes.HWND, wintypes.HWND, ctypes.c_int, ctypes.c_int,
                                           ctypes.c_int, ctypes.c_int, wintypes.UINT]
        self.user.ShowWindow.argtypes = [wintypes.HWND, ctypes.c_int]
        self._enum_callback_type = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
        self.user.EnumWindows.argtypes = [self._enum_callback_type, wintypes.LPARAM]
        self.root = tk.Tk()
        self.root.withdraw()
        self.root.overrideredirect(True)
        self.root.configure(bg="#010203")
        self.root.wm_attributes("-transparentcolor", "#010203")
        self.root.wm_attributes("-topmost", True)
        self.canvas = tk.Canvas(self.root, bg="#010203", highlightthickness=0, bd=0)
        self.canvas.pack(fill="both", expand=True)
        self.root.update_idletasks()
        self.hwnd = self.user.GetAncestor(self.root.winfo_id(), 2)
        style = self.user.GetWindowLongPtrW(self.hwnd, -20)
        # TOOLWINDOW | TRANSPARENT | LAYERED | NOACTIVATE. No focus/input capture.
        self.user.SetWindowLongPtrW(self.hwnd, -20, style | 0x80 | 0x20 | 0x80000 | 0x08000000)
        self._visible = False
        self._bounds = None
        self._window = None
        self._last_find = 0.
        self.settings = load_settings()
        self._image_sources, self._images = {}, {}
        self._menu_visible = False
        self._dismissed_menu = False
        self._panel = None
        self._variables = {}
        self._normal_style = style | 0x80 | 0x20 | 0x80000 | 0x08000000
        self.user.SetForegroundWindow.argtypes = [wintypes.HWND]
        root = Path(__file__).resolve().parents[1]
        try:
            manifest = json.loads((root / '.local/ui-assets/manifest.json').read_text(encoding='utf-8'))
            for asset in manifest['assets']:
                self._image_sources[asset['role']] = Image.open(root / asset['texture']).convert('RGBA')
        except (OSError, ValueError, KeyError):
            pass
        self.root.bind('<Escape>', lambda event: self._close_menu() if self._menu_visible else None)

    def _image(self, role, size):
        key = (role, int(size))
        if key not in self._images and role in self._image_sources:
            self._images[key] = ImageTk.PhotoImage(self._image_sources[role].resize((int(size), int(size)), Image.Resampling.LANCZOS))
        return self._images.get(key)

    def _command(self, action):
        payload = {'id': time.time_ns(), 'action': action}
        path = self.status_file.parent / 'ui-command.json'
        pending = path.with_suffix('.pending')
        pending.write_text(json.dumps(payload), encoding='utf-8')
        pending.replace(path)

    def _close_menu(self, apply=False, quit_game=False):
        if apply:
            self.settings = save_settings({name: var.get() for name, var in self._variables.items()})
        self._command('quit' if quit_game else 'resume')
        self._dismissed_menu = True
        self._menu_visible = False
        if self._panel: self._panel.place_forget()
        self.user.SetWindowLongPtrW(self.hwnd, -20, self._normal_style)
        if self._window and not quit_game: self.user.SetForegroundWindow(self._window)

    def _show_menu(self, width, height):
        if self._menu_visible: return
        self._menu_visible = True
        self.user.SetWindowLongPtrW(self.hwnd, -20, self._normal_style & ~0x20 & ~0x08000000)
        if self._panel is None:
            panel = self._panel = tk.Frame(self.root, bg='#172832', highlightbackground='#597078', highlightthickness=1)
            tk.Label(panel, text='设置', bg='#172832', fg='white', font=('Microsoft YaHei UI', 23, 'bold')).pack(pady=(18, 4))
            tk.Label(panel, text='斗牛 1 · 本地训练', bg='#172832', fg='#b4c7cc', font=('Microsoft YaHei UI', 10)).pack(pady=(0, 12))
            rows = tk.Frame(panel, bg='#172832'); rows.pack(fill='x', padx=32)
            specs = [('sensitivity','鼠标灵敏度',.05,2,.01), ('fov','视野',80,120,1),
                     ('hud_scale','界面大小',.7,1.4,.05), ('crosshair_size','准星线长',1,16,1),
                     ('crosshair_gap','准星间距',0,16,1)]
            for index,(name,label,low,high,step) in enumerate(specs):
                var = self._variables[name] = tk.DoubleVar(value=self.settings[name])
                tk.Label(rows, text=label, bg='#172832', fg='white', width=14, anchor='w', font=('Microsoft YaHei UI',11)).grid(row=index,column=0,sticky='w')
                tk.Scale(rows,variable=var,from_=low,to=high,resolution=step,orient='horizontal',length=300,
                         bg='#172832',fg='white',troughcolor='#30464e',highlightthickness=0,activebackground='#8df6d5').grid(row=index,column=1)
            for index,(name,label) in enumerate([('enemy_outline','黄绿色敌方轮廓'),('minimap','显示小地图'),('skill_hints','显示技能名称')],start=len(specs)):
                var=self._variables[name]=tk.BooleanVar(value=self.settings[name])
                tk.Checkbutton(rows,text=label,variable=var,bg='#172832',fg='white',selectcolor='#30464e',activebackground='#172832',activeforeground='white',font=('Microsoft YaHei UI',11)).grid(row=index,column=0,columnspan=2,sticky='w',pady=3)
            buttons=tk.Frame(panel,bg='#172832'); buttons.pack(pady=18)
            for title,action in [('保存并返回',lambda:self._close_menu(apply=True)),('返回',self._close_menu),('结束训练',lambda:self._close_menu(quit_game=True))]:
                tk.Button(buttons,text=title,command=action,bg='#34535c',fg='white',relief='flat',padx=18,pady=8,font=('Microsoft YaHei UI',10)).pack(side='left',padx=5)
        for name,var in self._variables.items(): var.set(self.settings[name])
        self._panel.place(x=width/2,y=height/2,anchor='center',width=560,height=565)
        self.root.focus_force()


    def _owner(self, hwnd):
        pid = wintypes.DWORD()
        self.user.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
        return pid.value

    def _find_game_window(self):
        matches = []
        def visit(hwnd, _):
            if self._owner(hwnd) != self.pid or not self.user.IsWindowVisible(hwnd):
                return True
            length = self.user.GetWindowTextLengthW(hwnd)
            title = ctypes.create_unicode_buffer(length+1)
            self.user.GetWindowTextW(hwnd, title, length+1)
            if title.value.strip().upper() == "VALORANT":
                matches.append(hwnd)
            return True
        callback = self._enum_callback_type(visit)
        self.user.EnumWindows(callback, 0)
        return matches[0] if len(matches) == 1 else None

    def _hide(self):
        if self._visible:
            self.user.ShowWindow(self.hwnd, 0)
            self._visible = False

    def _draw(self, state, width, height):
        c = self.canvas
        c.delete('all')
        self.settings = state.get('settings', self.settings)
        scale = min(width/1280., height/720.) * self.settings.get('hud_scale',1.)
        x,y=width/2,height/2
        phase=state.get('phase'); paused=bool(state.get('paused'))
        menu=bool(state.get('settings_open'))
        if not menu: self._dismissed_menu=False
        if phase=='running' and not paused and not menu:
            color='#ffb359' if state.get('hitmarker') else '#e9ffef'
            gap=self.settings.get('crosshair_gap',5); length=self.settings.get('crosshair_size',6)
            for ax,ay,bx,by in ((x-gap-length,y,x-gap,y),(x+gap,y,x+gap+length,y),
                               (x,y-gap-length,x,y-gap),(x,y+gap,x,y+gap+length)):
                c.create_line(ax,ay,bx,by,fill='#16251b',width=4)
                c.create_line(ax,ay,bx,by,fill=color,width=2)
            if state.get('hitmarker'):
                for a,b in ((-1,-1),(-1,1),(1,-1),(1,1)):
                    c.create_line(x+a*14,y+b*14,x+a*20,y+b*20,fill='#ffb359',width=3)
        if self.settings.get('minimap',True):
            size=210*scale
            minimap=self._image('duel1_map_revealed',size)
            if minimap and not state.get('native_ui_flags',0)&1: c.create_image(20+size/2,20+size/2,image=minimap)
            # Original MinimapCamera2_0 + native world-to-map matrix. Source:
            # reports/duel1-map-calibration.json, independently checked on 8 box edges.
            def project(position):
                u=.5+position[1]/12800.; v=.5-position[0]/12800.
                return (20+u*size,20+v*size) if 0<=u<=1 and 0<=v<=1 else None
            player=state.get('player',{})
            if player.get('position_cm'):
                point=project(player['position_cm'])
                if point:
                    yaw=math.radians(player.get('rotation',[0,0,0])[1])
                    dx,dy=math.sin(yaw),-math.cos(yaw)
                    px,py=point; radius=5*scale
                    c.create_polygon(px+dx*radius*1.6,py+dy*radius*1.6,
                                     px-dx*radius-dy*radius,py-dy*radius+dx*radius,
                                     px-dx*radius+dy*radius,py-dy*radius-dx*radius,fill='#8af8dd',outline='white')
            bot=state.get('bot')
            if bot and bot.get('health',0)>0:
                point=project(bot['position_cm'])
                if point:
                    bx,by=point; r=3*scale
                    c.create_oval(bx-r,by-r,bx+r,by+r,fill='#deff36',outline='#202b21')
            c.create_text(24,30+size,text='斗牛 1',anchor='nw',fill='white',font=('Microsoft YaHei UI',10))
        if phase=='running':
            base=height-77*scale
            c.create_text(x,height*.055,text=f"训练    {state.get('kills',0)} 击杀",fill='white',font=('Microsoft YaHei UI',int(16*scale),'bold'))
            hp=self._image('health_circle',64*scale)
            if hp: c.create_image(x-270*scale,base,image=hp)
            c.create_text(x-270*scale,base,text=str(state.get('player',{}).get('health',100)),fill='white',font=('Arial',int(24*scale),'bold'))
            skills=state.get('skills',{})
            for i,(name,role,label,key) in enumerate((('smoke','jett_cloudburst','瞬云','C'),('updraft','jett_updraft','凌空','Q'),('dash','jett_tailwind','逐风','E'),('ultimate','jett_blade_storm','飓刃','X'))):
                sx=x+(i-1.5)*76*scale; data=skills.get(name,{})
                icon=self._image(role,40*scale)
                if icon and not state.get('native_ui_flags',0)&2: c.create_image(sx,base-7*scale,image=icon)
                cooldown=data.get('cooldown',0); active=data.get('active',False)
                color='#79efd2' if active or cooldown<=0 else '#819494'
                c.create_line(sx-20*scale,base+20*scale,sx+20*scale,base+20*scale,fill=color,width=3)
                caption=f'{cooldown:.1f}' if cooldown>0 else str(data.get('knives','')) if name=='ultimate' and active else key
                c.create_text(sx,base+36*scale,text=caption,fill='white',font=('Arial',int(11*scale),'bold'))
                if self.settings.get('skill_hints',True):
                    c.create_text(sx,base-42*scale,text=label,fill='#d8e9e5',font=('Microsoft YaHei UI',int(10*scale)))
            ammo=state.get('ammo',0); maximum=state.get('magazine_size',25)
            c.create_text(x+270*scale,base,text=f'{ammo} / {maximum}',fill='white',font=('Arial',int(27*scale),'bold'))
            c.create_text(x+270*scale,base+30*scale,text='飓刃' if state.get('equipment_mode') else '狂徒',fill='#d8e9e5',font=('Microsoft YaHei UI',int(10*scale)))
        message={'waiting_for_native':'正在连接本地训练模块','validating_routes':'正在验证出场路线',
                 'error':'训练服务未就绪 · 查看运行记录','stopped':'训练已结束','stopping':'正在结束训练'}.get(phase,'')
        if phase=='running' and paused: message='训练已暂停 · Tab 继续'
        if phase=='running' and state.get('reloading'): message='换弹中'
        if message: c.create_text(x,y+55,text=message,fill='white',font=('Microsoft YaHei UI',14,'bold'))
        c.create_text(width-20,height-18,text='R 换弹    C / Q / E / X 技能    Tab 暂停    Esc 设置',anchor='se',fill='#c9d9d5',font=('Microsoft YaHei UI',9))
        if menu and not self._dismissed_menu:
            c.create_rectangle(0,0,width,height,fill='#10232d',stipple='gray50',outline='')
            self._show_menu(width,height)
        elif self._menu_visible:
            self._menu_visible=False
            self._panel.place_forget()
            self.user.SetWindowLongPtrW(self.hwnd,-20,self._normal_style)

    def tick(self):
        try:
            if not self.watch.alive():
                self.root.destroy()
                return
            now = time.monotonic()
            if not self._window or now-self._last_find > 1:
                self._window = self._find_game_window()
                self._last_find = now
            foreground = self.user.GetForegroundWindow()
            try:
                state = json.loads(self.status_file.read_text(encoding="utf-8"))
            except (OSError, ValueError):
                state = {}
            allowed_foreground = self._owner(foreground) == self.pid or (state.get('settings_open') and self._owner(foreground)==os.getpid())
            if not self._window or not allowed_foreground or self.user.IsIconic(self._window):
                self._hide()
                self.root.after(50,self.tick)
                return
            if state.get("pid") != self.pid or time.time()-state.get("updated_at", 0) > 30:
                self._hide()
                self.root.after(50, self.tick)
                return
            rect, point = wintypes.RECT(), wintypes.POINT(0, 0)
            if not self.user.GetClientRect(self._window, ctypes.byref(rect)) or not self.user.ClientToScreen(self._window, ctypes.byref(point)):
                self._hide()
                self.root.after(50, self.tick)
                return
            width, height = rect.right-rect.left, rect.bottom-rect.top
            if width <= 0 or height <= 0:
                self._hide()
                self.root.after(50, self.tick)
                return
            bounds = (point.x, point.y, width, height)
            if bounds != self._bounds:
                self.root.geometry(f"{width}x{height}{point.x:+d}{point.y:+d}")
                self.canvas.configure(width=width, height=height)
                self._bounds = bounds
            if not self._visible:
                self.root.deiconify()
                self.user.ShowWindow(self.hwnd, 8)  # SW_SHOWNA.
                self._visible = True
            self.user.SetWindowPos(self.hwnd, wintypes.HWND(-1), *bounds, 0x0010 | 0x0040)
            self._draw(state, width, height)
            self.root.after(33, self.tick)
        except tk.TclError:
            return

    def run(self):
        try:
            self.root.after(0, self.tick)
            self.root.mainloop()
        finally:
            self.watch.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pid", type=int, required=True)
    parser.add_argument("--status", type=Path, required=True)
    args = parser.parse_args()
    TrainingHUD(args.pid, args.status.resolve()).run()


if __name__ == "__main__":
    main()
