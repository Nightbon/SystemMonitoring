\mainpage SystemMonitor

# SystemMonitor

Документация по архитектуре, API и тестированию системы мониторинга SystemMonitor.

\dot
digraph ClassDiagram {
    rankdir=TB;
    compound=true;
    node [shape=record, style=filled, fontname="Verdana", fontsize=10];
    edge [fontname="Verdana", fontsize=9, arrowhead=onormal];

    subgraph cluster_model {
        label="MODEL LAYER"; color=forestgreen; style=bold;

        Metric [label="{Metric|+ name : string\l+ value : MetricValue (variant)\l+ operator\<=\> : default\l}", fillcolor=palegreen];

        AgentConfig [label="{AgentConfig|+ name : string\l+ type : AgentType\l+ metrics : vector\<string\>\l+ thresholds : vector\<CriticalThreshold\>\l+ update_interval_seconds : int\l+ config_path : string\l+ operator\<=\> : default\l}", fillcolor=palegreen];

        CriticalThreshold [label="{CriticalThreshold|+ metric_name : string\l+ op : string\l+ value : double\l+ operator\<=\> : default\l}", fillcolor=palegreen];

        AgentType [label="{AgentType|kCpu\lkMemory\lkNetwork\lkUnknown\l}", fillcolor=palegreen];

        IAgent [label="{IAgent (abstract)|+ UpdateMetrics() : vector\<Metric\>\l+ GetConfig() : AgentConfig\&\l+ SetConfig(config)\l+ GetName() : string\l+ GetType() : AgentType\l+ GetStartTime() : time_t\l+ SetActive(bool)\l+ IsActive() : bool\l}", fillcolor=palegreen];

        BaseAgent [label="{BaseAgent|+ GetConfig() : AgentConfig\&\l+ SetConfig(config)\l+ GetName() : string\l+ GetType() : AgentType\l+ GetStartTime() : time_t\l+ SetActive(bool)\l+ IsActive() : bool\l# config_ : AgentConfig\l# start_time_ : time_t\l# active_ : bool\l}", fillcolor=palegreen];

        Logger [label="{Logger|+ LogMetrics(span\<Metric\>)\l+ GetLastLines(count) : vector\<string\>\l- logs_dir_ : path\l- current_date_ : string\l- file_stream_ : ofstream\l- mutex_ : shared_mutex\l}", fillcolor=palegreen];

        ConfigParser [label="{ConfigParser (static)|+ Parse(path) : AgentConfig\l+ Save(path, config)\l}", fillcolor=palegreen];

        CriticalChecker [label="{CriticalChecker (static)|+ Compare(val, op, thr) : bool\l+ Check(metrics, thresholds) : vector\<string\>\l}", fillcolor=palegreen];

        AgentLoader [label="{AgentLoader|+ Load(so_path, config_path) : LoadedAgent\l}", fillcolor=palegreen];

        LoadedAgent [label="{LoadedAgent|+ agent : shared_ptr\<IAgent\>\l+ so_path : path\l}", fillcolor=palegreen];

        AgentManager [label="{AgentManager|+ StartScanning(interval)\l+ StopScanning()\l+ ScanAndLoad()\l+ GetAgents() : vector\<shared_ptr\<IAgent\>\>\l+ RemoveAgent(name)\l- agents_dir_ : path\l- agents_ : vector\<LoadedAgent\>\l- mutex_ : shared_mutex\l- scan_thread_ : jthread\l- scan_interval_ : int\l- loader_ : AgentLoader\l}", fillcolor=palegreen];

        TelegramBot [label="{TelegramBot|+ SendAlert(alert) : bool\l+ SetCredentials(token, chat_id)\l+ IsConfigured() : bool\l- token_ : string\l- chat_id_ : string\l}", fillcolor=palegreen];

        EmailSender [label="{EmailSender|+ SendAlert(alert) : bool\l+ SetCredentials(...)\l+ SetRecipient(to)\l+ SetEnabled(bool)\l+ IsConfigured() : bool\l+ IsEnabled() : bool\l- smtp_server_ : string\l- smtp_port_ : int\l- enabled_ : bool\l}", fillcolor=palegreen];

        AlertNotifier [label="{AlertNotifier|+ ProcessAlerts(alerts)\l+ SetTelegramCredentials(token, chat_id)\l+ SetEmailCredentials(...)\l+ SetEmailRecipient(to)\l+ SetEmailEnabled(bool)\l- telegram_bot_ : TelegramBot\l- email_sender_ : EmailSender\l}", fillcolor=palegreen];

        MonitorCore [label="{MonitorCore|+ Start()\l+ Stop()\l+ GetAgents() : vector\<shared_ptr\<IAgent\>\>\l+ GetLastLogLines(count) : vector\<string\>\l+ UpdateAgentConfig(name, config)\l+ DisableAgent(name)\l+ SetTelegramCredentials(token, chat_id)\l+ SetEmailCredentials(...)\l+ SetEmailRecipient(to)\l+ SetEmailEnabled(bool)\l- agent_manager_ : AgentManager\l- logger_ : Logger\l- alert_notifier_ : AlertNotifier\l- worker_thread_ : jthread\l- update_interval_ : int\l- default_interval_ : int\l- WorkLoop(stop_token)\l}", fillcolor=palegreen];

        AgentConfig -> CriticalThreshold [arrowhead=vee];
        AgentConfig -> AgentType [arrowhead=vee];
        IAgent -> AgentConfig [arrowhead=vee];
        IAgent -> Metric [arrowhead=vee];
        BaseAgent -> IAgent [arrowhead=empty, style=bold];
        LoadedAgent -> IAgent [arrowhead=vee];
        AgentManager -> LoadedAgent [arrowhead=diamond, label=" owns"];
        AgentManager -> AgentLoader [arrowhead=vee];
        MonitorCore -> AgentManager [arrowhead=diamond, label=" owns"];
        MonitorCore -> Logger [arrowhead=diamond, label=" owns"];
        MonitorCore -> AlertNotifier [arrowhead=diamond, label=" owns"];
        AlertNotifier -> TelegramBot [arrowhead=diamond, label=" owns"];
        AlertNotifier -> EmailSender [arrowhead=diamond, label=" owns"];
        MonitorCore -> CriticalChecker [arrowhead=vee, style=dashed];
        MonitorCore -> ConfigParser [arrowhead=vee, style=dashed];
    }

    subgraph cluster_agents {
        label="AGENTS (.so plugins)"; color=orange; style=bold;

        CpuAgent [label="{CpuAgent|+ UpdateMetrics() : vector\<Metric\>\l}", fillcolor=lightyellow];
        MemoryAgent [label="{MemoryAgent|+ UpdateMetrics() : vector\<Metric\>\l}", fillcolor=lightyellow];
        NetworkAgent [label="{NetworkAgent|+ UpdateMetrics() : vector\<Metric\>\l}", fillcolor=lightyellow];

        CpuAgent -> BaseAgent [arrowhead=empty, style=dashed];
        MemoryAgent -> BaseAgent [arrowhead=empty, style=dashed];
        NetworkAgent -> BaseAgent [arrowhead=empty, style=dashed];
    }

    subgraph cluster_controller {
        label="CONTROLLER LAYER"; color=blue; style=bold;

        MonitorController [label="{MonitorController (QObject)|+ Start()\l+ Stop()\l+ GetAgents() : vector\<shared_ptr\<IAgent\>\>\l+ GetLastLogLines(count) : vector\<string\>\l+ DisableAgent(name)\l+ UpdateAgentConfig(name, config)\l+ SetTelegramCredentials(...)\l+ SetEmailCredentials(...)\l+ SetEmailRecipient(to)\l+ SetEmailEnabled(bool)\l- core_ : MonitorCore*\l- poll_timer_ : QTimer*\l}", fillcolor=lightblue];

        MonitorController -> MonitorCore [arrowhead=vee, label=" uses"];
    }

    subgraph cluster_view {
        label="VIEW LAYER (Qt6)"; color=purple; style=bold;

        MainWindow [label="{MainWindow (QMainWindow)|+ OnAgentsChanged()\l+ OnLogUpdated()\l+ OnAgentSelected()\l+ OnDisableAgent()\l+ OnConfigureAgent()\l- SetupUi()\l- UpdateAgentDetails(agent)\l- controller_ : MonitorController*\l- agent_list_ : QListWidget*\l- log_view_ : QTextEdit*\l}", fillcolor=plum1];

        AgentConfigDialog [label="{AgentConfigDialog (QDialog)|+ GetConfig() : AgentConfig\l- SetupUi(config)\l- name_edit_ : QLineEdit*\l- type_combo_ : QComboBox*\l- metrics_list_ : QListWidget*\l- interval_spin_ : QSpinBox*\l- thresholds_table_ : QTableWidget*\l}", fillcolor=plum1];

        MainWindow -> AgentConfigDialog [arrowhead=vee, style=dashed];
    }

    MonitorController -> MainWindow [arrowhead=vee, label=" signals", color=red, fontcolor=red, style=dashed];
    MainWindow -> MonitorController [arrowhead=vee, label=" calls", color=blue, fontcolor=blue, style=dashed];
}
\enddot

## 1. Общее описание

Модель (Model) системы мониторинга реализована на языке C++ стандарта **C++20** в пространстве имён `s21`. Код придерживается **Google Style**, активно использует идиому **RAII** и стандартные средства потокобезопасности (`std::shared_mutex`, `std::jthread` с `std::stop_token`).

Модель отвечает за:
- Динамическую загрузку агентов из разделяемых библиотек (`.so`);
- Периодический сбор метрик от активных агентов через `std::jthread` и `std::stop_token`;
- Потокобезопасное логирование в ежедневные файлы с ротацией;
- Проверку критических порогов и рассылку уведомлений (Telegram, Email);
- Фоновое сканирование каталога агентов на появление новых `.so`-библиотек;
-Graceful shutdown через механизм `std::stop_token` (C++20).

---

## 2. Архитектура модели

```
┌──────────────────────────────────────────────────────────────────────┐
│                           MonitorCore                                 │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  ┌──────────┐ │
│  │ AgentManager │  │    Logger    │  │CriticalCheck │  │  Alert   │ │
│  │  (jthread)   │  │(shared_mutex)│  │              │  │Notifier  │ │
│  └──────┬───────┘  └──────┬───────┘  └──────────────┘  └────┬─────┘ │
│         │                 │                                │        │
│  ┌──────┴───────┐  ┌──────┴───────┐              ┌────────┴──────┐ │
│  │  ./agents/   │  │  ./logs/     │              │ TelegramBot   │ │
│  │  .so + .conf │  │ YYYY-MM-DD   │              │ EmailSender   │ │
│  └──────────────┘  └──────────────┘              └───────────────┘ │
│         │                                                         │
│  ┌──────┴───────┐                                                  │
│  │ AgentLoader  │                                                  │
│  │  dlopen/dlsym│                                                  │
│  └──────┬───────┘                                                  │
│         │                                                          │
│  ┌──────┴───────┐     ┌───────────────────────────────────────────┐ │
│  │  BaseAgent   │────>│ IAgent (interface)                        │ │
│  │  (abstract)  │     │  UpdateMetrics() -> vector<Metric>        │ │
│  └──────┬───────┘     │  GetConfig(), SetConfig()                 │ │
│         │             │  GetName(), GetType(), IsActive()         │ │
│  ┌──────┴──────────┐  └───────────────────────────────────────────┘ │
│  │ CpuAgent        │                                               │
│  │ MemoryAgent     │  WorkLoop (jthread + stop_token):             │
│  │ NetworkAgent    │  1. filter active agents (IsActive)           │
│  │ (test_plugin)   │  2. min_element by update_interval            │
│  └─────────────────┘  3. for each: UpdateMetrics() + Check()      │
│                       4. LogMetrics if !empty                      │
│                       5. ProcessAlerts if !empty                   │
│                       6. sleep_for(interval)                       │
└──────────────────────────────────────────────────────────────────────┘
```

---

## 3. Структуры данных

### 3.1. `metric.h`

```cpp
using MetricValue = std::variant<int, double, std::string>;

struct Metric {
  std::string name;
  MetricValue value;

  auto operator<=>(const Metric&) const = default;
};
```

- **MetricValue** — типобезопасный вариант для значений метрик (C++20 `std::variant`).
- **Metric** — пара «имя метрики / значение» с автоматически сгенерированным оператором сравнения (`operator<=>`).
- Используется для передачи данных от агента к ядру и в логгер.

Дополнительно определён концепт `MetricValueType`, ограничивающий допустимые типы значений метрик:

```cpp
template <typename T>
concept MetricValueType = std::is_arithmetic_v<T> || std::same_as<T, std::string>;
```

---

### 3.2. `agent_config.h`

```cpp
enum class AgentType { kCpu, kMemory, kNetwork, kUnknown };

struct CriticalThreshold {
  std::string metric_name;  // имя метрики, например "cpu"
  std::string op;           // оператор: "<", "<=", "==", ">=", ">"
  double value = 0.0;       // пороговое значение

  auto operator<=>(const CriticalThreshold&) const = default;
};

struct AgentConfig {
  std::string name;                          // имя агента
  AgentType type = AgentType::kUnknown;      // тип
  std::vector<std::string> metrics;          // список отслеживаемых метрик
  std::vector<CriticalThreshold> thresholds; // критические пороги
  int update_interval_seconds = 5;           // период обновления
  std::string config_path;                   // путь к .conf файлу

  auto operator<=>(const AgentConfig&) const = default;
};

constexpr const char* AgentTypeToString(AgentType type) noexcept;
constexpr int GetDefaultInterval() noexcept { return 5; }
```

- Все три структуры имеют `operator<=>` по умолчанию для сравнения и использования в алгоритмах STL.
- `AgentTypeToString` — `constexpr`-функция преобразования enum в строку ("CPU", "Memory", "Network", "Unknown").
- `GetDefaultInterval` — возвращает стандартный интервал обновления (5 секунд).

---

## 4. Интерфейс агента

### 4.1. `i_agent.h`

Абстрактный базовый класс, который должен реализовать каждый агент-`.so`:

```cpp
class IAgent {
 public:
  virtual ~IAgent() = default;
  [[nodiscard]] virtual std::vector<Metric> UpdateMetrics() = 0;
  [[nodiscard]] virtual const AgentConfig& GetConfig() const = 0;
  virtual void SetConfig(const AgentConfig& config) = 0;
  [[nodiscard]] virtual std::string GetName() const = 0;
  [[nodiscard]] virtual AgentType GetType() const = 0;
  [[nodiscard]] virtual std::time_t GetStartTime() const = 0;
  virtual void SetActive(bool active) = 0;
  [[nodiscard]] virtual bool IsActive() const = 0;
};
```

| Метод | `[[nodiscard]]` | Назначение |
|---|---|---|
| `UpdateMetrics()` | да | Собирает актуальные метрики и возвращает их вектор |
| `GetConfig()` | да | Возвращает текущую конфигурацию агента |
| `SetConfig()` | нет | Применяет новую конфигурацию (включая пороги) |
| `GetName()` | да | Имя агента для отображения в UI |
| `GetType()` | да | Тип агента (`kCpu`, `kMemory`, `kNetwork`) |
| `GetStartTime()` | да | Время запуска агента (`time_t`) |
| `SetActive()` | нет | Вкл/выкл сбор метрик |
| `IsActive()` | да | Текущее состояние активности |

### 4.2. `base_agent.h`

Базовая реализация `IAgent`, от которой наследуются все конкретные агенты. Реализует общую логику: хранение конфигурации, времени старта, флага активности.

```cpp
class BaseAgent : public IAgent {
 public:
  BaseAgent(const std::string& config_path, const std::string& default_name,
            AgentType default_type, std::vector<std::string> default_metrics);

  const AgentConfig& GetConfig() const override;
  void SetConfig(const AgentConfig& config) override;
  std::string GetName() const override;
  AgentType GetType() const override;
  std::time_t GetStartTime() const override;
  void SetActive(bool active) override;
  bool IsActive() const override;

 protected:
  AgentConfig config_;
  std::time_t start_time_;
  bool active_;
};
```

Конкретные агенты (`CpuAgent`, `MemoryAgent`, `NetworkAgent`) наследуют `BaseAgent` и реализуют только `UpdateMetrics()`.

### 4.3. Требования к `.so`

Каждая библиотека агента должна экспортировать **C-интерфейс**:

```cpp
extern "C" s21::IAgent* create_agent(const char* config_path);
```

- `config_path` — путь к `.conf` файлу агента.
- Функция должна вернуть указатель на объект, наследующий `s21::BaseAgent` (через `IAgent`).
- Ядро управляет временем жизни объекта через `std::shared_ptr` с кастомным удалителем, который вызывает `delete` и `dlclose`.

---

## 5. Компоненты модели

### 5.1. `ConfigParser`

**Файлы:** `config_parser.h` / `config_parser.cc`

Статический класс для чтения и записи конфигурационных файлов агентов.

```cpp
class ConfigParser {
 public:
  [[nodiscard]] static AgentConfig Parse(const std::string& file_path);
  static void Save(const std::string& file_path, const AgentConfig& config);
};
```

#### Формат `.conf`:

```ini
name=cpu_agent
type=cpu
metrics=cpu,processes
update_interval=5
critical_cpu=>=90
critical_processes=>100
```

| Ключ | Описание |
|---|---|
| `name` | Имя агента |
| `type` | `cpu`, `memory`, `network` |
| `metrics` | Список через запятую |
| `update_interval` | Период в секундах (целое) |
| `critical_<metric>` | Порог: оператор + число (`>=90`, `<50`, `==100`) |

**Поддерживаемые операторы:** `<`, `<=`, `==`, `>=`, `>`

---

### 5.2. `AgentLoader`

**Файлы:** `agent_loader.h` / `agent_loader.cc`

Загружает динамические библиотеки через `dlopen`/`dlsym`.

```cpp
struct LoadedAgent {
  std::shared_ptr<IAgent> agent;  // RAII-обёртка: delete + dlclose
  std::filesystem::path so_path;  // путь к .so
};

class AgentLoader {
 public:
  [[nodiscard]] LoadedAgent Load(const std::filesystem::path& so_path,
                                 const std::filesystem::path& config_path);
};
```

**RAII-стратегия:**
- `shared_ptr` хранит кастомный удалитель: `delete ptr; dlclose(handle);`.
- Гарантирует освобождение ресурсов даже при исключениях.
- Все пути передаются как `std::filesystem::path` (C++17).

---

### 5.3. `AgentManager`

**Файлы:** `agent_manager.h` / `agent_manager.cc`

Управляет жизненным циклом агентов и фоновым сканированием.

```cpp
class AgentManager {
 public:
  explicit AgentManager(std::filesystem::path agents_dir);
  ~AgentManager();

  void StartScanning(int interval_seconds);
  void StopScanning();
  void ScanAndLoad();

  [[nodiscard]] std::vector<std::shared_ptr<IAgent>> GetAgents() const;
  void RemoveAgent(const std::string& agent_name);

 private:
  std::filesystem::path agents_dir_;
  std::vector<LoadedAgent> agents_;
  mutable std::shared_mutex mutex_;
  std::jthread scan_thread_;
  int scan_interval_ = GetDefaultInterval();
  AgentLoader loader_;

  void ScanLoop(std::stop_token stop_token);
  [[nodiscard]] bool IsAlreadyLoaded(const std::filesystem::path& path) const;
};
```

**Поведение:**
- В отдельном `std::jthread` с `std::stop_token` обходит каталог агентов.
- Ищет файлы с расширением `.so`.
- Для каждого `.so` ищет одноимённый `.conf` в том же каталоге.
- Через `AgentLoader::Load()` создаёт агент и добавляет в `agents_`.
- `IsAlreadyLoaded()` предотвращает повторную загрузку одного и того же `.so`.
- Все операции с вектором агентов защищены `std::shared_mutex` (читатели-писатели).

---

### 5.4. `Logger`

**Файлы:** `logger.h` / `logger.cc`

Потокобезопасный логгер с ежедневной ротацией файлов.

```cpp
class Logger {
 public:
  explicit Logger(std::filesystem::path logs_dir);
  ~Logger();

  void LogMetrics(std::span<const Metric> metrics);
  [[nodiscard]] std::vector<std::string> GetLastLines(int count) const;

 private:
  std::filesystem::path logs_dir_;
  std::string current_date_;
  std::ofstream file_stream_;
  mutable std::shared_mutex mutex_;

  [[nodiscard]] std::string GetCurrentDate() const;
  [[nodiscard]] std::string GetCurrentTimestamp() const;
  void OpenFileForDate(const std::string& date);
};
```

**Формат записи:**
```
[yy-MM-dd HH:mm:ss] | Metric1 : Value1 | Metric2 : Value2 | ...
```

Пример:
```
[26-06-07 14:30:00] | cpu : 45.50 | processes : 142 | ram : 62.30
```

**Особенности:**
- Файлы создаются в указанном каталоге логов.
- Имя файла: `YYYY-MM-DD.log` (полная дата).
- При смене даты автоматически открывается новый файл (`OpenFileForDate`).
- `std::shared_mutex` защищает запись и чтение (множественные читатели).
- `GetLastLines(count)` возвращает последние `count` строк текущего дня.
- `LogMetrics` принимает `std::span<const Metric>` (C++20, без лишних копий).

---

### 5.5. `CriticalChecker`

**Файлы:** `critical_checker.h` / `critical_checker.cc`

Проверяет числовые метрики на выход за критические пороги.

```cpp
class CriticalChecker {
 public:
  [[nodiscard]] static bool Compare(double value, const std::string& op,
                                    double threshold);
  [[nodiscard]] static std::vector<std::string> Check(
      std::span<const Metric> metrics,
      std::span<const CriticalThreshold> thresholds);
};
```

**Алгоритм `Check`:**
1. Для каждого порога ищет метрику с совпадающим именем.
2. Извлекает числовое значение через `std::visit` (поддерживает `int`, `double`).
3. Сравнивает с порогом через `Compare()` (поддерживает операторы `<`, `<=`, `==`, `>=`, `>`).
4. При срабатывании формирует строку:
   ```
   CRITICAL: <metric> = <value> (threshold <op> <threshold_value>)
   ```

**Возвращает:** вектор строк с алертами. Если пороги не сработали — пустой вектор.

---

### 5.6. `TelegramBot`

**Файлы:** `telegram_bot.h` / `telegram_bot.cc`

Отправка алертов в Telegram через Bot API.

```cpp
class TelegramBot {
 public:
  TelegramBot() = default;
  explicit TelegramBot(std::string token, std::string chat_id);

  void SetCredentials(std::string token, std::string chat_id);
  [[nodiscard]] bool IsConfigured() const;
  bool SendAlert(const std::string& alert);

 private:
  std::string token_;
  std::string chat_id_;
};
```

- Использует `libcurl` для HTTP-запросов к `https://api.telegram.org/bot<token>/sendMessage`.
- `IsConfigured()` возвращает `true`, если заданы и токен, и chat_id.
- Без настроенных credentials `SendAlert` возвращает `false` без отправки.

---

### 5.7. `EmailSender`

**Файлы:** `email_sender.h` / `email_sender.cc`

Отправка алертов по email через SMTP (libcurl).

```cpp
class EmailSender {
 public:
  EmailSender() = default;
  explicit EmailSender(std::string smtp_server, int smtp_port,
                       std::string username, std::string password,
                       std::string from_address);

  void SetCredentials(std::string smtp_server, int smtp_port,
                      std::string username, std::string password,
                      std::string from_address);
  void SetRecipient(std::string to_address);
  void SetEnabled(bool enabled);

  [[nodiscard]] bool IsConfigured() const;
  [[nodiscard]] bool IsEnabled() const;
  bool SendAlert(const std::string& alert);

 private:
  std::string smtp_server_;
  int smtp_port_ = 587;
  std::string username_;
  std::string password_;
  std::string from_address_;
  std::string to_address_;
  bool enabled_ = false;
};
```

- Поддерживает SMTP с STARTTLS (порт 587 по умолчанию).
- `IsConfigured()` проверяет наличие SMTP-credentials.
- `IsEnabled()` проверяет флаг `enabled_`.
- `SendAlert` отправляет только при `IsConfigured() && IsEnabled()`.

---

### 5.8. `AlertNotifier`

**Файлы:** `alert_notifier.h` / `alert_notifier.cc`

Агрегирует каналы уведомлений (Telegram + Email) и маршрутизирует алерты.

```cpp
class AlertNotifier {
 public:
  void SetTelegramCredentials(const std::string& token, const std::string& chat_id);
  void SetEmailCredentials(const std::string& smtp_server, int smtp_port,
                           const std::string& username, const std::string& password,
                           const std::string& from_address);
  void SetEmailRecipient(const std::string& to_address);
  void SetEmailEnabled(bool enabled);

  void ProcessAlerts(const std::vector<std::string>& alerts);

 private:
  TelegramBot telegram_bot_;
  EmailSender email_sender_;
};
```

**Поведение `ProcessAlerts`:**
- Для каждого алерта пытается отправить через Telegram и Email.
- Если канал не настроен — пропускается без ошибки.
- Возвращает `true`, если хотя бы один канал успешно отправил.

---

### 5.9. `MonitorCore`

**Файлы:** `monitor_core.h` / `monitor_core.cc`

Центральное ядро системы мониторинга.

```cpp
class MonitorCore {
 public:
  MonitorCore(std::filesystem::path agents_dir, std::filesystem::path logs_dir,
              int update_interval_seconds);
  ~MonitorCore();

  void Start();
  void Stop();

  [[nodiscard]] std::vector<std::shared_ptr<IAgent>> GetAgents() const;
  [[nodiscard]] std::vector<std::string> GetLastLogLines(int count) const;

  void UpdateAgentConfig(const std::string& agent_name, const AgentConfig& config);
  void DisableAgent(const std::string& agent_name);

  void SetTelegramCredentials(const std::string& token, const std::string& chat_id);
  void SetEmailCredentials(const std::string& smtp_server, int smtp_port,
                           const std::string& username, const std::string& password,
                           const std::string& from_address);
  void SetEmailRecipient(const std::string& to_address);
  void SetEmailEnabled(bool enabled);

 private:
  AgentManager agent_manager_;
  Logger logger_;
  AlertNotifier alert_notifier_;
  std::jthread worker_thread_;
  int update_interval_;
  int default_interval_;

  void WorkLoop(std::stop_token stop_token);
};
```

**Жизненный цикл:**

1. **Конструктор** — инициализирует `AgentManager`, `Logger`, `AlertNotifier`. Сохраняет `default_interval_`.
2. **Start()** — запускает фоновое сканирование агентов (`AgentManager::StartScanning`) и рабочий поток `std::jthread` с `WorkLoop`.
3. **WorkLoop(stop_token)** — основной рабочий цикл:
   - Получает список агентов через `AgentManager::GetAgents()`.
   - Фильтрует активных через `std::views::filter` + `IsActive()`.
   - Находит минимальный `update_interval_seconds` через `std::ranges::min_element`.
   - Если активных агентов нет — спит на `default_interval_` и продолжает цикл.
   - Для каждого активного агента:
     - Вызывает `UpdateMetrics()`, результат перемещает в `all_metrics` через `std::ranges::move`.
     - Вызывает `CriticalChecker::Check(all_metrics, thresholds)`.
     - Алерты перемещаются в `all_alerts`.
     - Исключения отдельного агента перехватываются `catch(...)` — цикл продолжается.
   - Если `!all_metrics.empty()` — вызывает `logger_.LogMetrics(all_metrics)`.
   - Если `!all_alerts.empty()` — вызывает `alert_notifier_.ProcessAlerts(all_alerts)`.
   - Глобальный `catch(...)` защищает весь цикл от непредвиденных ошибок.
   - Спит на `update_interval_` секунд.
   - Проверяет `stop_token.stop_requested()` для graceful shutdown.
4. **Stop()** — вызывает `worker_thread_.request_stop()` и `agent_manager_.StopScanning()`. Деструктор автоматически вызывает `Stop()`.
5. **UpdateAgentConfig(name, config)** — ищет агент по имени через `std::ranges::find`, вызывает `SetConfig()` и `ConfigParser::Save()`.

---

## 6. Потокобезопасность

| Компонент | Механизм | Защищаемые данные |
|---|---|---|
| `AgentManager` | `std::shared_mutex` | Вектор `agents_` (читатели-писатели) |
| `Logger` | `std::shared_mutex` | Поток `file_stream_`, чтение строк |
| `MonitorCore` | `std::jthread` + `stop_token` | Graceful shutdown рабочего потока |
| `AgentManager` | `std::jthread` + `stop_token` | Graceful shutdown сканирования |

**Стратегия остановки (C++20):**
- `std::jthread` автоматически вызывает `request_stop()` в деструкторе.
- `WorkLoop` и `ScanLoop` проверяют `stop_token.stop_requested()` на каждой итерации.
- `MonitorCore::Stop()` явно вызывает `request_stop()` + `StopScanning()`.

---

## 7. Контроллер (Controller)

### 7.1. `MonitorController`

**Файлы:** `src/controller/monitor_controller.h` / `.cc`

Тонкий контроллер, связывающий Model и View. Не содержит бизнес-логики и UI-кода.

```cpp
class MonitorController : public QObject {
  Q_OBJECT
 public:
  explicit MonitorController(MonitorCore* core, QObject* parent = nullptr);
  ~MonitorController();

  void Start();
  void Stop();

  std::vector<std::shared_ptr<IAgent>> GetAgents() const;
  std::vector<std::string> GetLastLogLines(int count) const;
  void DisableAgent(const std::string& name);
  void UpdateAgentConfig(const std::string& name, const AgentConfig& config);

  void SetTelegramCredentials(const std::string& token, const std::string& chat_id);
  void SetEmailCredentials(const std::string& smtp_server, int smtp_port,
                           const std::string& username, const std::string& password,
                           const std::string& from_address);
  void SetEmailRecipient(const std::string& to_address);
  void SetEmailEnabled(bool enabled);

 signals:
  void AgentsChanged();
  void LogUpdated();

 private slots:
  void OnPoll();

 private:
  MonitorCore* core_;
  QTimer* poll_timer_;
};
```

| Метод | Назначение |
|---|---|
| `GetAgents()` | Прокси к `MonitorCore::GetAgents()` |
| `GetLastLogLines(count)` | Прокси к `MonitorCore::GetLastLogLines()` |
| `DisableAgent(name)` | Прокси к `MonitorCore::DisableAgent()` + сигнал `AgentsChanged` |
| `UpdateAgentConfig(name, config)` | Прокси к `MonitorCore::UpdateAgentConfig()` + сигнал `AgentsChanged` |
| `SetTelegramCredentials(...)` | Прокси к `MonitorCore::SetTelegramCredentials()` |
| `SetEmailCredentials(...)` | Прокси к `MonitorCore::SetEmailCredentials()` |
| `SetEmailRecipient(to)` | Прокси к `MonitorCore::SetEmailRecipient()` |
| `SetEmailEnabled(bool)` | Прокси к `MonitorCore::SetEmailEnabled()` |

**Polling:** `QTimer` с периодом 500 мс опрашивает модель через слот `OnPoll()` и эмитирует `AgentsChanged` / `LogUpdated`. Это позволяет View оставаться пассивной и не знать о внутреннем устройстве модели.

---

## 8. Представление (View)

### 8.1. `MainWindow`

**Файлы:** `src/view/main_window.h` / `.cc`

Главное окно приложения на Qt6. Не содержит бизнес-логики.

**Компоновка:**
- `QSplitter` (горизонтальный)
  - **Левая панель:**
    - `QListWidget` — список подключённых агентов
    - Кнопки: **Disable Agent**, **Configure**
    - `QGroupBox` «Agent Details» — поля: Name, Type, Metrics, Uptime, Update Interval
  - **Правая панель:**
    - `QTextEdit` (read-only, Monospace) — последние 20 строк лога

**Поведение:**
- При `AgentsChanged` — обновляется список агентов, сохраняется текущий выбор.
- При `LogUpdated` — запрашиваются последние 20 строк через контроллер и отображаются. Автопрокрутка вниз.
- При выборе агента — `OnAgentSelected()` находит агента по имени и обновляет детали через `UpdateAgentDetails()`.
- **Disable Agent** — вызывает `controller_->DisableAgent()`, список обновится по сигналу.
- **Configure** — открывает `AgentConfigDialog` с текущим конфигом. При `Accepted` — `controller_->UpdateAgentConfig()`.

### 8.2. `AgentConfigDialog`

**Файлы:** `src/view/agent_config_dialog.h` / `.cc`

Диалог редактирования конфигурации агента.

**Поля:**
- `QLineEdit` — имя агента
- `QComboBox` — тип (CPU / Memory / Network)
- `QListWidget` — список метрик (редактируемый, последняя строка `<new>` для добавления)
- `QSpinBox` — интервал обновления (1-3600 с)
- `QTableWidget` (3 столбца: Metric, Op, Value) — критические пороги

**Кнопки:** OK / Cancel (`QDialogButtonBox`)

---

## 9. Unit-тесты (GTest/GMock)

**Расположение:** `src/tests/`
**Всего тестов:** 217

| Тестовый файл | Кол-во тестов | Что проверяется |
|---|---|---|
| `test_config_parser.cc` | 38 | Парсинг `Parse()`, сохранение `Save()`, round-trip, пустые/битые файлы, все операторы порогов |
| `test_monitor_core.cc` | 36 | Start/Stop, GetAgents, GetLastLogLines, UpdateAgentConfig (с реальным .so-плагином), WorkLoop (сбор метрик, CriticalChecker, Logger, AlertNotifier, исключения, sleep) |
| `test_critical_checker.cc` | 23 | Все операторы (`<`, `<=`, `==`, `>=`, `>`), множественные пороги, отсутствие срабатываний |
| `test_email_sender.cc` | 20 | Конфигурация, SendAlert, включение/выключение, не настроенный sender |
| `test_agent_manager.cc` | 17 | Сканирование, загрузка, повторная загрузка, удаление, остановка сканирования |
| `test_logger.cc` | 18 | Запись `LogMetrics()`, чтение `GetLastLines()`, ротация файлов, пустые/отрицательные count |
| `test_base_agent.cc` | 16 | Конструктор, GetConfig/SetConfig, GetName, GetType, GetStartTime, SetActive/IsActive |
| `test_agent_config.cc` | 14 | `operator<=>`, AgentTypeToString, GetDefaultInterval, значения по умолчанию |
| `test_metric.cc` | 14 | Конструктор, `operator<=>`, variant с int/double/string |
| `test_alert_notifier.cc` | 11 | ProcessAlerts, SetTelegramCredentials, SetEmailCredentials, SetEmailEnabled |
| `test_telegram_bot.cc` | 10 | SetCredentials, IsConfigured, SendAlert |
| `mock_agent.h` | — | `MockAgent` (GMock) — мок всех 8 методов `IAgent` |
| `test_plugin_agent.cc` | — | Исходник тестового .so-плагина (управляется через `/tmp/s21_plugin_control.txt`) |

**Инфраструктура тестирования:**
- **GTest/GMock** собран из исходников как статическая библиотека (`/tmp/libgtest.a`).
- **Тестовый .so-плагин** (`test_plugin.so`) — реальный динамически загружаемый агент, управляемый через файл `/tmp/s21_plugin_control.txt`. Поддерживает команды: `cpu=<val>,ram=<val>`, `throw`, `empty`.
- **Makefile** (`src/tests/Makefile`) — компилирует модель + тесты + GTest, линкует с `-lcurl -ldl -lpthread`.

---

## 10. Сборка (CMake + Makefile)

### 10.1. CMakeLists.txt (корень проекта)

- **Стандарт:** C++20 (`CMAKE_CXX_STANDARD 20`)
- **Qt6:** автоматическая генерация MOC через `CMAKE_AUTOMOC ON`
- **Опции CMake:**
  - `Release=ON` — сборка приложения `SystemMonitor`
  - `BUILD_TESTING=ON` — сборка тестов `run_tests`
  - `BUILD_DVI=ON` — генерация Doxygen-документации
- **Линковка:** `Qt6::Core`, `Qt6::Widgets`, `dl`, `pthread`

### 10.2. Makefile (src/)

| Цель | Действие |
|---|---|
| `all` / `build` | `cmake -DRelease=ON -DBUILD_TESTING=OFF` + `make` |
| `test` | `cmake -DBUILD_TESTING=ON` + `make` + `./run_tests` |
| `gcov_report` | Сборка с `--coverage`, запуск тестов, HTML-отчёт `gcovr` |
| `clean` | `rm -rf ../build/` |
| `install` | Копирование `SystemMonitor` в `$(PREFIX)/SystemMonitor` |
| `uninstall` | Удаление установленного бинарника |
| `dvi` | `cmake -DBUILD_DVI=ON` + `make doxygen` |
| `dist` | Архивация `src/` + `CMakeLists.txt` в `../dist/SystemMonitor.tar.gz` |
| `style` | `clang-format -n --style=Google` для всех `.cc` / `.h` |
| `cppcheck` | Статический анализ модели, контроллера, вида и `main.cc` |
| `valgrind` | `valgrind --leak-check=full ./run_tests` |
| `check` | Последовательно: `gcov_report` + `style` + `cppcheck` + `valgrind` |

### 10.3. Makefile (src/tests/)

| Цель | Действие |
|---|---|
| `all` / `run_tests` | Компиляция модели + тестов + GTest, линковка в `run_tests` |
| `run` | Сборка и запуск `./run_tests --gtest_color=yes` |
| `clean` | Очистка `.o` и `run_tests` |

**Переменные:**
- `GTEST_DIR` — путь к GoogleTest (по умолчанию `/tmp/googletest`).
- `GTEST_LIB` — путь к собранной статической библиотеке (по умолчанию `/tmp/libgtest.a`).

---

## 11. Пример использования модели

```cpp
#include "model/monitor_core.h"
#include <iostream>

int main() {
  s21::MonitorCore core("./agents", "./logs", 5);

  // Настройка уведомлений
  core.SetTelegramCredentials("BOT_TOKEN", "CHAT_ID");
  core.SetEmailCredentials("smtp.gmail.com", 587,
                           "user@gmail.com", "password", "sender@gmail.com");
  core.SetEmailRecipient("admin@example.com");
  core.SetEmailEnabled(true);

  core.Start();

  // ... работа приложения ...

  core.Stop();
  return 0;
}
```

---

## 12. Требования к агентам по типам

### 12.1. CPU-агент (`type=cpu`)

| Метрика | Тип | Описание |
|---|---|---|
| `cpu` | `double` | Загрузка процессора (%) |
| `processes` | `int` | Количество процессов |

### 12.2. Memory-агент (`type=memory`)

| Метрика | Тип | Описание |
|---|---|---|
| `ram_total` | `double` | Общий объём ОЗУ |
| `ram` | `double` | Загрузка ОЗУ (%) |
| `hard_volume` | `double` | Использование диска |
| `hard_ops` | `int` | Операции ввода-вывода диска в секунду |
| `hard_throughput` | `double` | Пропускная способность диска |

### 12.3. Network-агент (`type=network`)

| Метрика | Тип | Описание |
|---|---|---|
| `<url>` | `int` | Доступность адреса: `0` — недоступен, `1` — доступен |
| `inet_throughput` | `double` | Пропускная способность сетевых интерфейсов |

---

## 13. Соответствие требованиям ТЗ

| Требование ТЗ | Реализация |
|---|---|
| C++20 | `std=c++20` в Makefile, `std::variant`, `std::filesystem`, `std::jthread`, `std::stop_token`, `std::ranges`, `std::span`, concepts |
| Пространство `s21` | Все классы в `namespace s21` |
| Google Style | Именование, отступы, структура файлов |
| RAII | `shared_ptr` с кастомным удалителем, `std::lock_guard`, `std::ofstream` RAII, `std::jthread` |
| Makefile: `all`, `install`, `uninstall`, `clean`, `dvi`, `dist`, `tests` | Полный набор целей |
| Динамические агенты `.so` | `AgentLoader` через `dlopen`/`dlsym` |
| Произвольное количество агентов | `std::vector<LoadedAgent>` в `AgentManager` |
| Динамическое подключение/отключение | `ScanAndLoad()`, `RemoveAgent()` |
| Сканирование `./agents/` | `AgentManager` с фоновым `std::jthread` |
| Единый лог-файл на день | `Logger` с именем `YYYY-MM-DD.log` |
| Формат лога | `[yy-MM-dd HH:mm:ss] \| Metric : Value \| ...` |
| Конфиг агента: имя, тип, метрики, пороги, интервал | `AgentConfig` + `ConfigParser` |
| Критические значения: `<, <=, ==, >=, >` | `CriticalChecker` с полным набором операторов |
| 3 типа агентов | `AgentType::kCpu`, `kMemory`, `kNetwork` |
| Unit-тесты GTest | `src/tests/` — 11 тестовых файлов + mock + тестовый .so-плагин (217 тестов) |
| Потокобезопасность | `std::shared_mutex`, `std::jthread`, `std::stop_token` |
| Уведомления | `AlertNotifier` → `TelegramBot` + `EmailSender` (SMTP) |