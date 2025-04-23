from nicegui import ui
import pandas as pd
import plotly.graph_objects as go
import time
import pandas as pd

default_display_range = 60  # Default display range in minutes

def store_data(kiln_sections, existing_df=None):
    """Store data from kiln sections into a single row in a DataFrame and append it to an existing DataFrame."""
    data = {
        'time': time.strftime('%Y-%m-%d %H:%M:%S'),
        'setpoint': kiln_sections[0].temperature_setpoint  # Assuming all sections share the same setpoint
    }

    # Add data for each kiln section
    for i, kiln in enumerate(kiln_sections, start=1):
        data[f's{i}_temp'] = kiln.temperature
        data[f's{i}_power'] = kiln.power_level
        data[f's{i}_enabled'] = kiln.enabled

    # Convert the single row of data into a DataFrame
    new_row_df = pd.DataFrame([data])

    # Append to the existing DataFrame if provided
    if existing_df is not None:
        updated_df = pd.concat([existing_df, new_row_df], ignore_index=True)
        # Remove rows older than 24 hours
        updated_df['time'] = pd.to_datetime(updated_df['time'])
        cutoff_time = pd.Timestamp.now() - pd.Timedelta(hours=24)
        updated_df = updated_df[updated_df['time'] > cutoff_time]
        return updated_df
    else:
        new_row_df['time'] = pd.to_datetime(new_row_df['time'])
        return new_row_df
    
    
def plotting_tab(data_log):
    '''Create a for the setpoint and each kiln temperature for each time stamp'''
    global plot
    global fig
    fig = go.Figure()
    plot = ui.plotly(fig)
    fig.update_layout(
        xaxis=dict(
            rangeslider=dict(
                visible=True
            ),
            
        )
    )
    
global pause_log
pause_log = False
    
def pause_logging(e):
    global pause_log
    pause_log = e
    
def update_graph(data_log):
    """Update the graph with new data from the data log."""
    fig.data = []  # Clear the data for the next update
    fig.add_trace(go.Scatter(x=data_log['time'], y=data_log['setpoint'], name='Setpoint', mode='lines'))
    for i in range(1, len(data_log.columns) // 3 + 1):
        #print(data_log[f's{i}_enabled'])
        if data_log[f's{i}_enabled'].iloc[-1]:
            fig.add_trace(go.Scatter(x=data_log['time'], y=data_log[f's{i}_temp'], name=f'S{i} Temperature', mode='lines')) 
    x_range = [data_log['time'].min().strftime('%Y-%m-%d %H:%M:%S'), data_log['time'].max().strftime('%Y-%m-%d %H:%M:%S')]
    view_range = [(data_log['time'].max() - pd.Timedelta(minutes=default_display_range)).strftime('%Y-%m-%d %H:%M:%S'), data_log['time'].max().strftime('%Y-%m-%d %H:%M:%S')]
    
    fig.update_layout(
        xaxis=dict(
            rangeslider=dict(
                visible=True,
                range=x_range
            ),            
        ),
        margin=dict(l=10, r=10, t=15, b=10),
        autosize=False,
        width=800,
        height=400,
    )

    fig.update_traces(connectgaps=False)
    if pause_log == False:
        plot.update()
    

def export_to_csv(data_log, filename=None):
    """Export the data log to a CSV file."""
    if filename is None:
        filename = time.strftime('kiln_data_log_%Y-%m-%d_%H-%M-%S.csv')
    
    data_log.to_csv(filename, index=False)
    ui.notify(f'Data log exported to {filename}')
    