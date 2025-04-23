from nicegui import app, ui
import time
from kiln_control import KilnSection
import kiln_control as com
import control_tabs as ctrl
import data_logging as log
from plotly import graph_objects as go
import pandas as pd

# Export these to a separate file

kiln_sections = [
    KilnSection(section_id=1, port_name="COM14", temperature_setpoint=0, kC=200, kI=0.01, kD=500, gpio_pin=23, enabled=False),
    KilnSection(section_id=2, port_name="COM13", temperature_setpoint=0, kC=50, kI=0, kD=0, gpio_pin=24, enabled=False),
    KilnSection(section_id=3, port_name="COM11", temperature_setpoint=0, kC=50, kI=0, kD=0, gpio_pin=25, enabled=False),
    KilnSection(section_id=4, port_name="COM12", temperature_setpoint=0, kC=50, kI=0, kD=0, gpio_pin=8, enabled=False)
    ]
global data_log
data_log = pd.DataFrame(columns=['time', 'setpoint', 's1_temp', 's1_power', 's1_enabled', 's2_temp', 's2_power', 's2_enabled', 's3_temp', 's3_power', 's3_enabled', 's4_temp', 's4_power', 's4_enabled'])


# Tabs
with ui.footer().classes('pt-0 pb-0'):
    with ui.tabs() as tabs:
        tab1 = ui.tab("System Monitor & Control")
        #tab2 = ui.tab("Create Program") 
        tab3 = ui.tab("Settings")
        tab4 = ui.tab("Logs")


with ui.tab_panels(tabs, value=tab1):
    with ui.tab_panel(tab1):
        ctrl.monitor_tab(kiln_sections)
    #with ui.tab_panel(tab2):
        #ctrl.program_tab(kiln_sections)
    with ui.tab_panel(tab3):
        ctrl.settings_tab(kiln_sections)
    with ui.tab_panel(tab4):
        data_log = log.store_data(kiln_sections, data_log)
        log.plotting_tab(data_log)
        with ui.row():
            ui.button('Export to CSV', on_click=lambda: log.export_to_csv(data_log))
            ui.button('Clear Data Log', on_click=lambda: data_log.drop(data_log.index, inplace=True))
            ui.switch('Pause Logging', on_change=lambda e: log.pause_logging(e.value))
        
        
        

def update_cycle():
    global data_log
    com.comm_cycle(kiln_sections) # Send Setpoint, Receive Temp and Status
    data_log = log.store_data(kiln_sections, data_log) # Append data to DataFrame and update graph
    log.update_graph(data_log)
    
    
ui.timer(2, update_cycle)
ui.run()