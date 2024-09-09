#include <iostream>
#include <vector>
#include <tuple>
#include <memory>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <string>
#include <thread>
#include <chrono>
#include <random>
#include<algorithm>
#include<execution>
#include<fstream>
#include "csv.h"

using namespace std;

const int gobackSevenDays = 7 * 24 * 60 * 60;

// Abstract base class 
class weatherData {
private:
    std::vector<std::tuple<std::string, float, float, float, float, std::string>> dataset; //to store dataset elements

public:
    friend void printDataset(const weatherData& wd);

    friend void printSortedDataset(const weatherData& wd);

    virtual void parseCsv(const string& csvname) = 0;
    virtual void getRangeData(const string& start, const string& end) = 0;

    //setter method
    void addData(const std::string& date, float precipitation, float temp_max, float temp_min, float wind, const std::string& weather) {
        dataset.emplace_back(date, precipitation, temp_max, temp_min, wind, weather);
    }
    //getter method
    const std::vector<std::tuple<std::string, float, float, float, float, std::string>>& getDataset() const {
        return dataset;
    }
    virtual ~weatherData(){};
};

// Derived class to handle parsing and processing
class childWeatherData : public weatherData {
public:
    //to traverse csv file and compute overall statistics from the dataset 
    void parseCsv(const string& csvname) override {
        try {
            io::CSVReader<6> in(csvname);
            in.read_header(io::ignore_extra_column, "date", "precipitation", "temp_max", "temp_min", "wind", "weather");

            float totalTempSum = 0.0;
            float totalPrecipitation = 0.0;
            int count = 0;
            float maxTemp = -std::numeric_limits<float>::infinity();
            float minTemp = std::numeric_limits<float>::infinity();

            string date;
            float precipitation;
            float temp_max;
            float temp_min;
            float wind;
            string weather;

            while (in.read_row(date, precipitation, temp_max, temp_min, wind, weather)) {
                this->addData(date, precipitation, temp_max, temp_min, wind, weather);

                if (temp_max > maxTemp) {
                    maxTemp = temp_max;
                }
                if (temp_min < minTemp) {
                    minTemp = temp_min;
                }
                float avgTemp = (temp_max + temp_min) / 2.0;
                totalTempSum += avgTemp;
                totalPrecipitation += precipitation;
                count++;
            }

            if (count > 0) {
                cout<<endl;
                cout << "Statistics for entire \"" << csvname << "\"" << endl;
                cout << "Overall Average Temperature: " << totalTempSum / count << "C" << endl;
                cout << "Overall Average Precipitation: " << totalPrecipitation / count << " mm" << endl;
                cout << "Overall Maximum Temperature Recorded: " << maxTemp << "C" << endl;
                cout << "Overall Minimum Temperature Recorded: " << minTemp << "C" << endl;
            } else {
                cout << "No data found" << endl;
            }
        } catch (const exception& e) {
            cerr << e.what() << endl;
            exit(EXIT_FAILURE);
        }
    }
    //to compute statistics for a range of dates
    void getRangeData(const string& start, const string& end) override {
        const auto& dataset = getDataset();

        float totalTempSum = 0.0;
        float totalPrecipitation = 0.0;
        int count = 0;
        float maxTemp = -std::numeric_limits<float>::infinity();
        float minTemp = std::numeric_limits<float>::infinity();

        for (const auto& entry : dataset) {
            const auto& [date, precipitation, temp_max, temp_min, wind, weather] = entry;

            if (date >= start && date <= end) {
                if (temp_max > maxTemp) {
                    maxTemp = temp_max;
                }
                if (temp_min < minTemp) {
                    minTemp = temp_min;
                }
                float avgTemp = (temp_max + temp_min) / 2.0;
                totalTempSum += avgTemp;
                totalPrecipitation += precipitation;
                count++;
            }
        }

        if (count > 0) {
            cout<<endl;
            cout << "Statistics for range " << start << " - " << end << endl;
            cout << "Overall Average Temperature: " << totalTempSum / count << "C" << endl;
            cout << "Overall Average Precipitation: " << totalPrecipitation / count << " mm" << endl;
            cout << "Overall Maximum Temperature Recorded: " << maxTemp << "C" << endl;
            cout << "Overall Minimum Temperature Recorded: " << minTemp << "C" << endl;
        } else {
            cout << "No data found for the specified date range" << endl;
        }
    }
};

//Friend function to print entire dataset if explicitly called from main()
void printDataset(const weatherData& wd) {
    const auto& dataset = wd.getDataset();
    for (const auto& entry : dataset) {
        const auto& [date, precipitation, temp_max, temp_min, wind, weather] = entry;
        cout<<endl;
        cout << "Date: " << date
             << ", Precipitation: " << precipitation
             << ", Temp Max: " << temp_max
             << ", Temp Min: " << temp_min
             << ", Wind: " << wind
             << ", Weather: " << weather << endl;
    }
}

//Friend function to print entire sorted dataset(based on max_temp) if explicitly called from main()
void printSortedDataset(const weatherData& wd) {
    auto dataset = wd.getDataset();

    // Sort dataset based on temp_max 
    auto tempMaxComparator = [](const auto& lhs, const auto& rhs) {
        return std::get<2>(lhs) < std::get<2>(rhs);
    };
    //using parallel execution algorithm
    std::sort(std::execution::par, dataset.begin(), dataset.end(), tempMaxComparator);

    for (const auto& entry : dataset) {
        const auto& [date, precipitation, temp_max, temp_min, wind, weather] = entry;
        cout << endl;
        cout << "Date: " << date
             << ", Precipitation: " << precipitation
             << ", Temp Max: " << temp_max
             << ", Temp Min: " << temp_min
             << ", Wind: " << wind
             << ", Weather: " << weather << endl;
    }
}

// Convert a date string to std::tm to get the day 
std::tm stringToTm(const std::string& dateString, const std::string& format) {
    std::tm tm = {};
    std::istringstream ss(dateString);
    ss >> std::get_time(&tm, format.c_str());
    if (ss.fail()) {
        throw std::runtime_error("Failed to parse date string");
    }
    return tm;
}

// Convert std::tm to time_t for arithmetic operations
std::time_t tmToTimeT(const std::tm& tm) {
    return std::mktime(const_cast<std::tm*>(&tm));
}

// Get the last seven days dataset
std::vector<std::tuple<std::string, float, float, float, float, std::string>> getLastSevenDaysDataset(
    const std::vector<std::tuple<std::string, float, float, float, float, std::string>>& dataset) {

    if (dataset.empty()) return {};
    
    const std::string dateFormat = "%Y-%m-%d";
    std::string lastDateString = std::get<0>(dataset.back());
    std::tm lastDateTm = stringToTm(lastDateString, dateFormat);
    std::time_t lastDateTimeT = tmToTimeT(lastDateTm);
    std::time_t sevenDaysBeforeTimeT = lastDateTimeT - gobackSevenDays; // gobackSevenDays = 7 * 24 * 60 * 60;

    std::vector<std::tuple<std::string, float, float, float, float, std::string>> result;

    for (const auto& entry : dataset) {
        std::string dateString = std::get<0>(entry);
        std::tm dateTm = stringToTm(dateString, dateFormat);
        std::time_t dateTimeT = tmToTimeT(dateTm);

        if (dateTimeT >= sevenDaysBeforeTimeT) {
            result.push_back(entry);
        }
    }

    return result;
}

// Calculate tomorrows weather stats
std::string calculateTomorrowStats(
    const std::string& tomorrow_date, float tomorrow_precipitation, float tomorrow_max_temp,
    float tomorrow_min_temp, float tomorrow_wind,
    const std::vector<std::tuple<std::string, float, float, float, float, std::string>>& lastSevenDaysDataset) {

    float total_precipitation = 0.0f;
    float total_max_temp = 0.0f;
    float total_min_temp = 0.0f;
    float total_wind = 0.0f;
    float avg_precipitation = 0.0f, avg_max_temp = 0.0f, avg_min_temp = 0.0f, avg_wind = 0.0f;

    int count = lastSevenDaysDataset.size();

    for (const auto& entry : lastSevenDaysDataset) {
        total_precipitation += std::get<1>(entry);
        total_max_temp += std::get<2>(entry);
        total_min_temp += std::get<3>(entry);
        total_wind += std::get<4>(entry);
    }

    if (count > 0) {
        avg_precipitation = total_precipitation / count;
        avg_max_temp = total_max_temp / count;
        avg_min_temp = total_min_temp / count;
        avg_wind = total_wind / count;
    }

    // Determine the weather prediction
    if (tomorrow_precipitation > avg_precipitation + 5.0f && tomorrow_max_temp < avg_max_temp - 5.0f) {
        return "snow";
    }
    else if (tomorrow_precipitation > avg_precipitation + 2.0f && tomorrow_wind < avg_wind + 2.0f) {
        return "rain";
    }
    else if (tomorrow_precipitation < avg_precipitation && tomorrow_max_temp > avg_max_temp + 5.0f && tomorrow_min_temp > avg_min_temp + 5.0f) {
        return "sunny";
    }
    else if (tomorrow_wind > avg_wind + 5.0f && tomorrow_precipitation < avg_precipitation) {
        return "windy";
    }
    else if (tomorrow_precipitation > avg_precipitation && tomorrow_max_temp < avg_max_temp) {
        return "drizzle";
    }
    else {
        return "cloudy";
    }
}

// Daemon thread class to simulate daily weather updates
class dailyWeatherUpdate {
private:
    bool isRunning;

public:
    dailyWeatherUpdate() : isRunning(true) {}

    void start(weatherData& wd) {
        std::thread([this, &wd]() {
            while (isRunning) {
                // Simulate weather data generation for the day
                float precipitation = getRandomValue(0.0f, 50.0f);
                float temp_max = getRandomValue(-10.0f, 40.0f);
                float temp_min = getRandomValue(-20.0f, temp_max);
                float wind = getRandomValue(0.0f, 30.0f);

                auto lastDataset = wd.getDataset();
                if (lastDataset.empty()) {
                    std::cerr << "No historical data available to simulate weather update." << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(1)); 
                    continue;
                }

                std::string lastDate = std::get<0>(lastDataset.back());
                std::tm tm = stringToTm(lastDate, "%Y-%m-%d");
                tm.tm_mday += 1;
                std::mktime(&tm);

                std::ostringstream oss;
                oss << std::put_time(&tm, "%Y-%m-%d");
                std::string tomorrowDate = oss.str();

                std::vector<std::tuple<std::string, float, float, float, float, std::string>> lastSevenDaysDataset = getLastSevenDaysDataset(lastDataset);

                std::string predictedWeather = calculateTomorrowStats(tomorrowDate, precipitation, temp_max, temp_min, wind, lastSevenDaysDataset);

   
                wd.addData(tomorrowDate, precipitation, temp_max, temp_min, wind, predictedWeather);

            
                cout<<endl;
                cout << "Updated Weather Data for " << tomorrowDate << ": "
                     << "Precipitation: " << precipitation << " mm, "
                     << "Max Temp: " << temp_max << " C, "
                     << "Min Temp: " << temp_min << " C, "
                     << "Wind: " << wind << " km/h, "
                     << "Predicted Weather: " << predictedWeather << endl;

                // Wait for the next simulation cycle
                std::this_thread::sleep_for(std::chrono::seconds(1)); 
            }
        }).detach();
    }

    void stop() {
        isRunning = false;
    }

private:
    float getRandomValue(float minValue, float maxValue) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dist(minValue, maxValue);
        return dist(gen);
    }
};

int main() {
    unique_ptr<weatherData> wd = make_unique<childWeatherData>();
    string filename = "weather_dataset.csv";

    //file existence check
    std::ifstream file(filename);
    if (file) {
        file.close(); 

        try {
            wd->parseCsv(filename); 

            // Uncomment these lines to print the entire dataset or sorted dataset
            // printDataset(*wd); 
            // printSortedDataset(*wd);

            string startDate, endDate;
            cout << "Enter the start date (YYYY-MM-DD): ";
            cin >> startDate;
            cout << "Enter the end date (YYYY-MM-DD): ";
            cin >> endDate;

            wd->getRangeData(startDate, endDate);

            dailyWeatherUpdate weatherDaemon;
            weatherDaemon.start(*wd);

            cout << "Press Enter to stop the weather simulation..." << std::endl;
            cin.ignore(); // Wait for user input
            cin.get();
            weatherDaemon.stop();
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return EXIT_FAILURE;
        }
    } else {
        std::cerr << "No file found: " << filename << std::endl;
        return EXIT_FAILURE;
    }

    return 0;
}
