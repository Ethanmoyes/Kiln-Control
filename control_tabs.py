from nicegui import app, ui, events
import time
from io import StringIO
import kiln_control as com
import pandas as pd


# Export these to a separate file
     
def monitor_tab(kiln_sections):
    with ui.row():
        for section in kiln_sections:
            with ui.card().classes('p-1 gap-0'):
                ui.label(f"Section {section.section_id}")
                with ui.grid(columns=2, rows='auto').classes('gap-0 justify-items-center'):
                    ui.label("Setpoint").classes('gap-5 justify-self-start')
                    ui.label(section.temperature_setpoint).classes(' p-0').bind_text_from(section, "temperature_setpoint")
                    
                    ui.label("Temp").classes('gap-5 justify-self-start')
                    ui.label(section.temperature).classes(' p-1').bind_text_from(section, "temperature")
                    
                    ui.label("Section Enabled").classes('gap-5 justify-self-start')
                    ui.switch(value=section.enabled).classes('p-0').props('dense').bind_value(section, "enabled")
                    
                    ui.label("Percent Power").classes('gap-5 justify-self-start')
                    ui.label(section.power_level).classes(' p-0').bind_text_from(section, "power_level")
                    
                    ui.label("Status").classes('gap-5 justify-self-start')
                    ui.label("Disconnected").classes(' p-0').bind_text_from(section, "status")

    with ui.row():
        ui.label('Temp Setpoint')
        setpoint_value = ui.input(value=kiln_sections[0].temperature_setpoint).props('dense')
        ui.button('Update Setpoint', on_click=lambda: com.update_all_setpoints(kiln_sections, float(setpoint_value.value)))
        ui.switch('Enable/Disable All Sections', value=False, on_change=lambda e: com.toggle_kiln(kiln_sections, 0, e.value))

    
        
def settings_tab(kiln_sections):
    '''Settings Tab: Allows user to set PID values for all sections'''
    inital_kC = kiln_sections[0].kC
    inital_kI = kiln_sections[0].kI
    inital_kD = kiln_sections[0].kD
        
    ui.label('Set Control Parameters')
    with ui.row().classes('h-auto w-auto'):
        with ui.column():
            ui.label("kC")
            kc_val = ui.input(value=kiln_sections[0].kC, validation={'Float Value Only': lambda value: value.isdecimal()})
        with ui.column():
            ui.label("kI")
            ki_val = ui.input(value=kiln_sections[0].kI, validation={'Float Value Only': lambda value: value.isdecimal()})
        with ui.column():
            ui.label("kD")
            kd_val = ui.input(value=kiln_sections[0].kD, validation={'Float Value Only': lambda value: value.isdecimal()})
    with ui.row():
        def update_section(section_id):
            com.update_parameters(kiln_sections, section_id, float(kc_val.value), float(ki_val.value), float(kd_val.value))
            ui.notify(f"Updated Section {section_id} parameters: kC={kc_val.value}, kI={ki_val.value}, kD={kd_val.value}")
        def reset_to_defaults():
            com.update_parameters(kiln_sections, 0, inital_kC, inital_kI, inital_kD)
            kc_val.value = inital_kC
            ki_val.value = inital_kI
            kd_val.value = inital_kD
            ui.notify('Paremeters Reset')
        ui.button('Update All Sections', on_click=lambda: update_section(0))
        ui.button('Reset to Defaults', on_click=lambda: reset_to_defaults())

def program_tab(kiln_sections):
    '''Load a csv program'''
    df = pd.DataFrame()
    def toggle_uploader():
        uploader.visible = not uploader.visible
        load.visible = not load.visible
        if program_name_label.text != '':
            control_row.visible = True
        else:
            control_row.visible = False
    
    load = ui.button('Load a CSV Program', on_click=toggle_uploader)
    def handle_upload(event: events.UploadEventArguments):
        try:
            program_name_label.text = event.name
            with StringIO(event.content.read().decode()) as f:
                df = pd.read_csv(f)
            toggle_uploader()
        except Exception as e:
            ui.notify(f"Error loading file: {e}")
            return
        
    uploader = ui.upload(on_upload=handle_upload).props('accept=.csv')
    uploader.visible = False # Initially False to hide
    control_row = ui.row()
    control_row.visible=False # Initially False to hide
    with control_row:
        with ui.column():
            ui.label('Loaded Program')
            program_name_label = ui.label('')
        start_button = ui.button('Start Program').props('color=green')
        stop_button = ui.button('Stop Program').props('color=red')
        stop_button.visible = False

        def toggle_program():
            if start_button.text == 'Start Program':
                start_button.text = 'Pause Program'
                start_button.props('color=yellow')
                stop_button.visible = True
            
            # Add logic to start the program
            else:
                start_button.text = 'Start Program'
                start_button.props('color=green')
            # Add logic to pause the program
        def stop_program():
            start_button.text = 'Start Program'
            start_button.props('color=green')
            stop_button.visible = False
            # Add logic to stop the program

        start_button.on_click(toggle_program)
        stop_button.on_click(stop_program)

    
   