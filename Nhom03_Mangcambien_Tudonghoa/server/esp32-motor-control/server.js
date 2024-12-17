const express = require("express");
const cors = require("cors");
const bodyParser = require("body-parser");
const { MongoClient } = require("mongodb");
const path = require("path");
const moment = require("moment-timezone"); // Thêm moment-timezone
const XLSX = require("xlsx"); // Thư viện xuất Excel
const fs = require("fs"); // Thư viện để xử lý file

const app = express();
const port = 3000;

// Middleware
app.use(cors());
app.use(bodyParser.json());
app.use(express.static(path.join(__dirname, "public")));

// MongoDB Configuration
const uri = "mongodb://127.0.0.1:27017";
const dbName = "esp32_logs";
let db;

async function connectMongo() {
  try {
    const client = new MongoClient(uri);
    await client.connect();
    db = client.db(dbName);
    console.log("✅ Connected to MongoDB");
  } catch (error) {
    console.error("❌ MongoDB connection error:", error);
  }
}
connectMongo();

// API: ESP32 gửi log lên server
app.post("/log", async (req, res) => {
  const { log } = req.body;
  if (log) {
    console.log("Received log:", log);
    try {
      if (log.includes("Command received")) {
        const command = log.split(":")[1].trim();
        await db.collection("commandStatus").insertOne({
          command: command,
          updatedAt: new Date(),
        });
        console.log(`✅ Command saved: ${command}`);
      } else {
        const vietnamTime = moment().tz("Asia/Ho_Chi_Minh").format("YYYY-MM-DD HH:mm:ss");
        await db.collection("logs").insertOne({ message: log, timestamp: vietnamTime });
        console.log(`✅ Log saved: ${log}`);
      }
      res.status(200).send("Log received and saved");
    } catch (error) {
      console.error("❌ Error saving log:", error);
      res.status(500).send("Error saving log");
    }
  } else {
    res.status(400).send("Invalid log data");
  }
});

// API: Gửi log về giao diện web
app.get("/logs", async (req, res) => {
  try {
    const logs = await db.collection("logs").find({}).sort({ timestamp: -1 }).toArray();
    res.json(logs);
  } catch (error) {
    console.error("❌ Error fetching logs:", error);
    res.status(500).send("Error fetching logs");
  }
});

// API: Nhận lệnh điều khiển từ giao diện web
app.post("/control", async (req, res) => {
  const { newCommand } = req.body;
  if (["moveOut", "moveIn", "stop", "toggleMode"].includes(newCommand)) {
    try {
      await db.collection("commandStatus").insertOne({
        command: newCommand,
        updatedAt: new Date(),
      });
      console.log(`✅ Command inserted: ${newCommand}`);
      res.status(200).send(`Command set to: ${newCommand}`);
    } catch (error) {
      console.error("❌ Error saving command:", error);
      res.status(500).send("Error saving command");
    }
  } else {
    console.error("❌ Invalid command received:", newCommand);
    res.status(400).send("Invalid command");
  }
});

// API: Gửi trạng thái động cơ cho ESP32
app.get("/status", async (req, res) => {
  try {
    const latestStatus = await db
      .collection("commandStatus")
      .find({})
      .sort({ updatedAt: -1 })
      .limit(1)
      .toArray();
    const command = latestStatus[0]?.command || "stop";
    res.send(command);
  } catch (error) {
    console.error("❌ Error fetching command status:", error);
    res.status(500).send("Error fetching command status");
  }
});

// API: ESP32 gửi trạng thái mưa lên server
app.post("/rainStatus", async (req, res) => {
  const { rain } = req.body;
  if (rain) {
    try {
      await db.collection("rainStatus").insertOne({
        rainStatus: rain,
        updatedAt: new Date(),
      });
      console.log(`✅ Rain Status inserted: ${rain}`);
      res.status(200).send("Rain status inserted");
    } catch (error) {
      console.error("❌ Error saving rain status:", error);
      res.status(500).send("Error saving rain status");
    }
  } else {
    res.status(400).send("Invalid rain data");
  }
});

// API: Gửi trạng thái mưa cho giao diện web
app.get("/rainStatus", async (req, res) => {
  try {
    const latestRainStatus = await db
      .collection("rainStatus")
      .find({})
      .sort({ updatedAt: -1 })
      .limit(1)
      .toArray();
    res.json({ rainStatus: latestRainStatus[0]?.rainStatus || "Unknown" });
  } catch (error) {
    console.error("❌ Error fetching rain status:", error);
    res.status(500).send("Error fetching rain status");
  }
});

// API: Xuất dữ liệu ra file Excel (Cập nhật thời gian)
app.get("/export", async (req, res) => {
  try {
    const commandStatus = await db.collection("commandStatus").find({}).toArray();
    const logs = await db.collection("logs").find({}).toArray();
    const rainStatus = await db.collection("rainStatus").find({}).toArray();

    const formatDataWithTime = (data) =>
      data.map((item) => ({
        ...item,
        formattedTime: moment(item.updatedAt || item.timestamp || new Date())
          .tz("Asia/Ho_Chi_Minh")
          .format("YYYY-MM-DD HH:mm:ss"),
      }));

    const formattedCommandStatus = formatDataWithTime(commandStatus);
    const formattedLogs = formatDataWithTime(logs);
    const formattedRainStatus = formatDataWithTime(rainStatus);

    const wb = XLSX.utils.book_new();
    XLSX.utils.book_append_sheet(wb, XLSX.utils.json_to_sheet(formattedCommandStatus), "CommandStatus");
    XLSX.utils.book_append_sheet(wb, XLSX.utils.json_to_sheet(formattedLogs), "Logs");
    XLSX.utils.book_append_sheet(wb, XLSX.utils.json_to_sheet(formattedRainStatus), "RainStatus");

    const filePath = path.join(__dirname, "public", "data_export.xlsx");
    XLSX.writeFile(wb, filePath);

    res.download(filePath, "data_export.xlsx", (err) => {
      if (err) {
        console.error("❌ Error sending file:", err);
        res.status(500).send("Error exporting file");
      }
      fs.unlinkSync(filePath); // Xóa file tạm
    });
  } catch (error) {
    console.error("❌ Error exporting data:", error);
    res.status(500).send("Error exporting data");
  }
});

// API kiểm tra server
app.get("/", (req, res) => {
  res.send("Server ESP32 đang hoạt động!");
});

// Khởi động server
app.listen(port, () => {
  console.log(`✅ Server đang chạy tại http://localhost:${port}`);
});
