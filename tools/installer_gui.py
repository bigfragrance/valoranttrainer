"""Windows setup UI. Network starts only after an explicit install action."""
from __future__ import annotations

import argparse
import ctypes
import json
import os
from pathlib import Path
import queue
import shutil
import socket
import subprocess
import sys
import threading
import time
import tkinter as tk
from tkinter import filedialog, messagebox, ttk
import github_update

from installer_engine import Control, Installer, InstallationError, Plan, Stopped, Zstd, atomic_json, digest, extract_patch


CDN_OPTIONS = {'自动选线': 'auto', '默认 CDN': 'primary', 'Cloudflare': 'cloudflare',
               'CloudFront': 'cloudfront', 'Akamai': 'akamai'}
NETWORK_OPTIONS = {'系统代理（如有）': 'system', '直连': 'direct'}


def assets():
    return Path(getattr(sys, '_MEIPASS', Path(__file__).resolve().parents[1]/'.local/installer-build'))/'payload'


def gb(value):
    return f'{value/1000000000:.1f} GB'


def windows_root(root):
    root = Path(root)
    existing = root
    while not existing.exists() and existing != existing.parent:
        existing = existing.parent
    return existing


def prepare_environment(target, records, payload, descriptor):
    extract_patch(payload/'patch.zip', descriptor['patch_sha256'], target)
    atomic_json(target/'.setup/verified-files.json', {'files': records})
    environment = dict(os.environ)
    for key, folder in {'TEMP': 'temp', 'TMP': 'temp', 'LOCALAPPDATA': 'LocalAppData',
                        'APPDATA': 'Roaming', 'PYTHONPYCACHEPREFIX': 'pycache',
                        'UE-LocalDataCachePath': 'DerivedDataCache'}.items():
        directory = target/'.local/runtime'/folder
        directory.mkdir(parents=True, exist_ok=True)
        environment[key] = str(directory)
    process = subprocess.run([str(target/'runtime/python.exe'), '-I', '-B', '-X', 'utf8',
                              str(target/'tools/trainer_downloaded.py'), '--finalize-install'],
                             cwd=target, env=environment, capture_output=True, text=True, encoding='utf-8',
                             creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0), timeout=120)
    if process.returncode:
        raise InstallationError('补丁环境准备失败，资源进度已保留。'+(process.stderr or process.stdout)[-800:])


class SetupWindow:
    def __init__(self, root, payload, descriptor, target, *, preview=False):
        self.root, self.payload, self.descriptor = root, payload, descriptor
        self.preview = preview
        self.events = queue.Queue()
        self.worker = None
        self.control = None
        self.closing = False
        self.success = False
        self.success_target = None
        self.network_history = []
        self.target = tk.StringVar(value=str(target))
        self.concurrency = tk.IntVar(value=32)
        self.cdn_route = tk.StringVar(value='自动选线')
        self.network_mode = tk.StringVar(value='系统代理（如有）')
        self.last_progress = None
        root.title('ValorantTrainer 安装器')
        root.geometry('800x620')
        root.minsize(700, 600)
        root.configure(background='#f5f6f8')
        style = ttk.Style(root)
        style.theme_use('clam')
        style.configure('.', font=('Microsoft YaHei UI', 10), background='#f5f6f8', foreground='#202329')
        style.configure('TFrame', background='#f5f6f8')
        style.configure('Title.TLabel', font=('Microsoft YaHei UI', 22, 'bold'), foreground='#191c22')
        style.configure('Muted.TLabel', foreground='#69717e')
        style.configure('Value.TLabel', font=('Microsoft YaHei UI', 14, 'bold'))
        style.configure('TButton', padding=(14, 9), borderwidth=0, background='#e6e9ef')
        style.map('TButton', background=[('active', '#dce1e9'), ('disabled', '#eceef2')])
        style.configure('Primary.TButton', foreground='#ffffff', background='#1767d2', padding=(20, 10))
        style.map('Primary.TButton', background=[('active', '#1258b5'), ('disabled', '#9cbbe3')])
        style.configure('TEntry', padding=8, fieldbackground='#ffffff', bordercolor='#d8dde5')
        style.configure('Horizontal.TProgressbar', troughcolor='#e1e5ec', background='#1767d2', borderwidth=0, thickness=9)
        content = ttk.Frame(root, padding=(34, 24, 34, 18))
        content.pack(fill='both', expand=True)
        content.columnconfigure(0, weight=1)
        ttk.Label(content, text='安装离线训练器', style='Title.TLabel').grid(row=0, column=0, sticky='w')
        ttk.Label(content, text='完整地图、英雄和枪皮 · 英语音频 / 繁体中文文本',
                  style='Muted.TLabel').grid(row=1, column=0, sticky='w', pady=(5, 23))
        totals = ttk.Frame(content)
        totals.grid(row=2, column=0, sticky='ew', pady=(0, 23))
        totals.columnconfigure(0, weight=1)
        totals.columnconfigure(1, weight=1)
        ttk.Label(totals, text='预计下载', style='Muted.TLabel').grid(row=0, column=0, sticky='w')
        ttk.Label(totals, text='游戏文件占用', style='Muted.TLabel').grid(row=0, column=1, sticky='w')
        ttk.Label(totals, text=gb(descriptor['download_bytes']), style='Value.TLabel').grid(row=1, column=0, sticky='w', pady=(4, 0))
        ttk.Label(totals, text=gb(descriptor['game_bytes']), style='Value.TLabel').grid(row=1, column=1, sticky='w', pady=(4, 0))
        ttk.Label(totals, text='并发连接', style='Muted.TLabel').grid(row=0, column=2, sticky='w')
        self.connections = ttk.Combobox(totals, textvariable=self.concurrency, values=tuple(range(1, 65)),
                                        width=5, height=8, state='readonly')
        self.connections.grid(row=1, column=2, sticky='w', pady=(4, 0))
        ttk.Label(content, text='安装目录').grid(row=3, column=0, sticky='w', pady=(0, 7))
        folder = ttk.Frame(content)
        folder.grid(row=4, column=0, sticky='ew')
        folder.columnconfigure(0, weight=1)
        self.entry = ttk.Entry(folder, textvariable=self.target)
        self.entry.grid(row=0, column=0, sticky='ew', padx=(0, 9))
        self.browse = ttk.Button(folder, text='选择目录', command=self.choose)
        self.browse.grid(row=0, column=1)
        self.space = ttk.Label(content, text='', style='Muted.TLabel')
        self.space.grid(row=5, column=0, sticky='w', pady=(7, 20))
        self.status = ttk.Label(content, text='准备就绪，点击开始安装后下载资源', wraplength=710)
        self.status.grid(row=6, column=0, sticky='w', pady=(0, 12))
        self.progress = ttk.Progressbar(content, mode='determinate', maximum=100)
        self.progress.grid(row=7, column=0, sticky='ew')
        self.stats = ttk.Label(content, text='支持暂停续传和已有文件校验', style='Muted.TLabel')
        self.stats.grid(row=8, column=0, sticky='w', pady=(9, 0))
        content.rowconfigure(9, weight=1)
        network = ttk.Frame(content)
        network.grid(row=10, column=0, sticky='ew', pady=(15, 13))
        ttk.Label(network, text='CDN 线路').grid(row=0, column=0, padx=(0, 8))
        self.cdn_select = ttk.Combobox(network, textvariable=self.cdn_route, values=tuple(CDN_OPTIONS),
                                       width=13, state='readonly')
        self.cdn_select.grid(row=0, column=1, padx=(0, 18))
        ttk.Label(network, text='网络连接').grid(row=0, column=2, padx=(0, 8))
        self.network_select = ttk.Combobox(network, textvariable=self.network_mode, values=tuple(NETWORK_OPTIONS),
                                           width=17, state='readonly')
        self.network_select.grid(row=0, column=3)
        buttons = ttk.Frame(content)
        buttons.grid(row=11, column=0, sticky='ew')
        buttons.columnconfigure(0, weight=1)
        self.repair = ttk.Button(buttons, text='校验 / 修复', command=lambda: self.start(repair=True))
        self.repair.grid(row=0, column=0, sticky='w')
        self.pause = ttk.Button(buttons, text='暂停', command=self.toggle_pause, state='disabled')
        self.pause.grid(row=0, column=1, padx=(8, 8))
        self.install = ttk.Button(buttons, text='开始安装', style='Primary.TButton', command=self.start)
        self.install.grid(row=0, column=2)
        self.update_button = ttk.Button(buttons, text='GitHub 更新', command=self.start_update)
        self.update_button.grid(row=1, column=0, sticky='w', pady=(8, 0))
        self.target.trace_add('write', lambda *_: self.update_space())
        self.update_space()
        root.protocol('WM_DELETE_WINDOW', self.close)
        root.after(150, self.poll)
        if preview:
            self.status.configure(text='界面预览 · 下载功能已停用')
            self.install.configure(state='disabled')
            self.repair.configure(state='disabled')

    def choose(self):
        if self.worker and self.worker.is_alive():
            return
        selected = filedialog.askdirectory(title='选择训练器安装目录', initialdir=str(windows_root(self.target.get())))
        if selected:
            path = Path(selected)
            if (path.name.casefold() != 'valoranttrainer' and not (path/'.setup/installer-owner.json').is_file()
                    and not (path/'patch-version.json').is_file()):
                path /= 'ValorantTrainer'
            self.target.set(str(path))

    def update_space(self):
        if self.success and Path(self.target.get()).absolute() != self.success_target:
            self.success = False
            self.success_target = None
            self.install.configure(text='开始安装')
            self.progress['value'] = 0
            self.status.configure(text='准备就绪，点击开始安装后下载资源')
            self.stats.configure(text='支持暂停续传和已有文件校验')
        try:
            free = shutil.disk_usage(windows_root(self.target.get())).free
            self.space.configure(text='所在磁盘可用 '+gb(free)+' · 首次安装还需约 '+gb(self.descriptor['game_bytes']+512*1024*1024))
        except OSError:
            self.space.configure(text='请选择可写的本地磁盘目录')

    def start(self, repair=False):
        if self.preview or (self.worker and self.worker.is_alive()):
            return
        if self.success and not repair and Path(self.target.get()).absolute() == self.success_target:
            os.startfile(str(Path(self.target.get())/'启动训练器.cmd'))
            return
        target = Path(self.target.get()).absolute()
        workers = self.concurrency.get()
        cdn_route = CDN_OPTIONS[self.cdn_route.get()]
        network_mode = NETWORK_OPTIONS[self.network_mode.get()]
        self.success = False
        self.network_history.clear()
        self.last_progress = None
        self.control = Control()
        for widget in (self.entry, self.browse, self.repair, self.install, self.connections,
                       self.cdn_select, self.network_select):
            widget.configure(state='disabled')
        self.pause.configure(state='normal', text='暂停')
        self.status.configure(text='正在检查资源计划和已有文件')

        def work():
            try:
                codec = Zstd(self.payload/'libzstd.dll')
                installer = Installer(self.payload/'resource-plan.sqlite', self.descriptor, target, codec,
                    control=self.control, notify=self.events.put, workers=workers,
                    cdn_route=cdn_route, network_mode=network_mode,
                    prepare=lambda root, files: prepare_environment(root, files, self.payload, self.descriptor))
                result = installer.run()
                self.events.put({'phase': 'success', 'result': result, 'target': str(target)})
            except Stopped:
                self.events.put({'phase': 'stopped', 'message': '进度已保存，可再次继续安装'})
            except Exception as error:
                try:
                    atomic_json(target/'.setup/last-error.json', {'error': str(error), 'type': type(error).__name__})
                except OSError:
                    pass
                self.events.put({'phase': 'error', 'message': str(error)})
        self.worker = threading.Thread(target=work, daemon=False)
        self.worker.start()

    def start_update(self):
        if self.preview or (self.worker and self.worker.is_alive()):
            return
        target = Path(self.target.get()).absolute()
        channel = self.payload / 'github-update.json'
        repository = json.loads(channel.read_text(encoding='utf-8')).get('repository') if channel.is_file() else None
        self.status.configure(text='正在检查 GitHub 正式版本')
        def work():
            try:
                release = github_update.check(target, repository)
                self.events.put({'phase': 'update_check', 'release': release, 'target': str(target)})
            except Exception as error:
                self.events.put({'phase': 'error', 'message': str(error)})
        self.worker = threading.Thread(target=work, daemon=False)
        self.worker.start()

    def toggle_pause(self):
        if not self.control:
            return
        if self.control.running.is_set():
            self.control.pause()
            self.pause.configure(text='继续')
            self.status.configure(text='已暂停，下载进度保留在安装目录')
        else:
            self.control.resume()
            self.pause.configure(text='暂停')
            self.status.configure(text='正在继续安装')

    def close(self):
        if self.worker and self.worker.is_alive():
            self.closing = True
            if self.control:
                self.control.stop()
            self.status.configure(text='正在保存下载进度，请稍候')
            self.pause.configure(state='disabled')
            return
        self.root.destroy()

    def poll(self):
        while True:
            try:
                event = self.events.get_nowait()
            except queue.Empty:
                break
            phase = event['phase']
            if phase == 'update_check':
                release = event['release']
                if not release['available']:
                    self.status.configure(text='当前已是最新版本 ' + release['current'])
                elif messagebox.askyesno('训练器更新', '发现版本 ' + release['version'] + '\n关闭游戏和控制台后，是否下载并更新？'):
                    self.status.configure(text='正在下载并校验更新包')
                    def update_work(release=release, target=event['target']):
                        try:
                            github_update.install(Path(target), release)
                            self.events.put({'phase': 'stopped', 'message': '更新完成，请重新启动训练器'})
                        except Exception as error:
                            self.events.put({'phase': 'error', 'message': str(error)})
                    self.worker = threading.Thread(target=update_work, daemon=False)
                    self.worker.start()
            elif phase in ('success', 'error', 'stopped'):
                self.pause.configure(state='disabled')
                for widget in (self.entry, self.browse, self.repair, self.install):
                    widget.configure(state='normal')
                for widget in (self.connections, self.cdn_select, self.network_select):
                    widget.configure(state='readonly')
                self.last_progress = None
                if phase == 'success':
                    self.success = True
                    self.success_target = Path(event['target'])
                    self.status.configure(text='安装完成，资源和补丁环境已准备好')
                    self.install.configure(text='打开训练器')
                    self.progress['value'] = 100
                    self.stats.configure(text='全部游戏文件已通过校验')
                else:
                    self.status.configure(text=event['message'])
                    self.install.configure(text='继续安装')
            elif not self.closing and (not self.control or self.control.running.is_set()):
                self.status.configure(text=event['message'])
                total = event.get('total', self.descriptor['game_bytes'])
                completed = event.get('completed', 0)
                self.progress['value'] = min(100, completed/max(total, 1)*100)
                downloaded = event.get('downloaded', 0)
                value = gb(completed)+' / '+gb(total)
                if phase == 'download':
                    self.network_history.append((event.get('timestamp', time.monotonic()), downloaded))
                    self.last_progress = event
                else:
                    self.last_progress = None
                self.stats.configure(text=value)
        if self.last_progress and self.control and self.control.running.is_set():
            now = time.monotonic()
            self.network_history = [entry for entry in self.network_history if now-entry[0] <= 8]
            if self.network_history:
                first = self.network_history[0]
                speed = (self.last_progress['downloaded']-first[1])/max(now-first[0], 1)
            else:
                speed = 0
            self.stats.configure(text=f'网络 {speed/1000000:.1f} MB/s · 已写入 '
                +gb(self.last_progress['completed'])+' / '+gb(self.last_progress['total']))
        if self.closing and (not self.worker or not self.worker.is_alive()):
            self.root.destroy()
            return
        self.root.after(150, self.poll)


def self_test(payload, descriptor, report):
    # This entry point is deliberately disconnected from the download action.
    original = socket.create_connection
    def blocked(*_args, **_kwargs):
        raise AssertionError('Self-test must not connect to any network server')
    socket.create_connection = blocked
    root = None
    try:
        plan = Plan(payload/'resource-plan.sqlite', descriptor['plan_sha256'])
        assert len(plan.files) == descriptor['file_count'], 'Resource plan count differs'
        plan.close()
        assert digest(payload/'patch.zip') == descriptor['patch_sha256'], 'Embedded patch differs'
        codec = Zstd(payload/'libzstd.dll')
        assert codec.library is not None
        root = tk.Tk()
        root.withdraw()
        window = SetupWindow(root, payload, descriptor, Path(report).parent/'PreviewTrainer', preview=True)
        root.update_idletasks()
        requested = [root.winfo_reqwidth(), root.winfo_reqheight()]
        assert requested[0] <= 800 and requested[1] <= 620, 'Window content needs '+str(requested)
        assert window.install.instate(['disabled']) and window.repair.instate(['disabled']), 'Preview download controls must be disabled'
        assert window.concurrency.get() == 32, 'Default concurrency must be 32'
        assert [int(value) for value in window.connections.cget('values')] == list(range(1, 65)), 'Concurrency choices must cover 1 through 64'
        assert CDN_OPTIONS[window.cdn_route.get()] == 'auto', 'CDN route must default to automatic selection'
        assert NETWORK_OPTIONS[window.network_mode.get()] == 'system', 'System proxy selection must be available'
        window.success = True
        window.success_target = Path(window.target.get()).absolute()
        window.install.configure(text='打开训练器')
        window.target.set(str(Path(report).parent/'AnotherPreviewTrainer'))
        assert not window.success and window.install.cget('text') == '开始安装', 'Changing the target must reset the install action'
        atomic_json(report, {'passed': True, 'file_count': descriptor['file_count'],
            'plan_sha256': descriptor['plan_sha256'], 'patch_sha256': descriptor['patch_sha256'],
            'tk_initialized': True, 'zstd_loaded': True, 'network_connections': 0,
            'game_started': False, 'requested_size': [root.winfo_reqwidth(), root.winfo_reqheight()]})
    finally:
        socket.create_connection = original
        if root:
            root.destroy()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--target', type=Path, required=True)
    parser.add_argument('--self-test', type=Path)
    parser.add_argument('--preview', action='store_true')
    args = parser.parse_args()
    payload = assets()
    descriptor = json.loads((payload/'installer.json').read_text(encoding='utf-8'))
    if args.self_test:
        self_test(payload, descriptor, args.self_test)
        return
    if os.name == 'nt':
        try:
            ctypes.windll.shcore.SetProcessDpiAwareness(1)
        except (AttributeError, OSError):
            pass
    root = tk.Tk()
    SetupWindow(root, payload, descriptor, args.target, preview=args.preview)
    root.mainloop()


if __name__ == '__main__':
    try:
        main()
    except Exception as error:
        if '--self-test' in sys.argv:
            index = sys.argv.index('--self-test')
            atomic_json(Path(sys.argv[index+1]), {'passed': False, 'error': str(error)})
        else:
            from tkinter import messagebox
            messagebox.showerror('安装器启动失败', str(error))
        raise SystemExit(1)
