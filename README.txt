Weather Data Analyzer
Overview
The Weather Data Analyzer is a C++ application that parses historical weather data from a CSV file, performs data analysis over specified date ranges, and simulates daily weather updates in the background. 
The simulated weather updates are based on historical data trends, providing a prediction for the next day's weather.

Features
CSV Parsing: Reads and processes historical weather data from a CSV file.
Data Analysis: Computes overall statistics like average temperature, precipitation, etc.
Weather Simulation: Runs a daemon thread that simulates daily weather updates, adding them to the dataset and predicting the weather for the next day based on the last seven days' data.
Customizable Date Range: Users can specify start and end dates to retrieve and analyze specific subsets of the dataset.

Run the Application (Windows)
cd weather_data_analyzer
g++ .\main.cpp -o output
.\output.exe

For a much detailed understanding please refer documentation.pdf  