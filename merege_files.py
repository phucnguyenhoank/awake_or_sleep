import pandas as pd
import glob
import os

def merge_csv_files(folder_path, output_file):
    # Find all CSV files matching the pattern
    csv_files = glob.glob(os.path.join(folder_path, "phuc_*.csv"))
    
    # Load and concatenate all CSV files
    all_data = pd.concat((pd.read_csv(file) for file in csv_files), ignore_index=True)
    
    # Convert 'Timestamp' to datetime format for accurate sorting
    all_data['Timestamp'] = pd.to_datetime(all_data['Timestamp'])
    
    # Sort the data by Timestamp
    all_data = all_data.sort_values(by='Timestamp').reset_index(drop=True)
    
    # Save the combined DataFrame to a new CSV file
    all_data.to_csv(output_file, index=False)
    print(f"Merged file saved as: {output_file}")

# Example usage
merge_csv_files('./aos_data', './aos_data/merged_phuc_data.csv')
