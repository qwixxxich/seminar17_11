#include <array>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct Question {
    std::string stage;
    std::string title;
    std::string text;
    std::string code;
    std::array<std::string, 3> options;
    int correctNumber;
    std::string explanation;
};

const int POINTS_PER_ANSWER = 10;
const int INPUT_ENDED = -1;

const std::string STAGE_1 = "Опасная копия";
const std::string STAGE_2 = "Правильное владение";
const std::string STAGE_3 = "Передача ресурсов";
const std::string STAGE_4 = "Операторы под контролем";

std::vector<Question> createQuestions() {
    std::vector<Question> questions;

    const std::string rawArrayCode =
        "struct RawArray {\n"
        "    int* data = new int[3]{10, 20, 30};\n"
        "    ~RawArray() { delete[] data; }\n"
        "};\n"
        "\n"
        "RawArray a;\n"
        "RawArray b = a;\n"
        "b.data[0] = 99;";

    questions.push_back({
        STAGE_1,
        "Копирование указателя",
        "Выполняется RawArray b = a. Что делает неявный конструктор копирования с полем data?",
        rawArrayCode,
        {
            "Создаёт новый буфер и копирует в него элементы.",
            "Копирует значение указателя: оба объекта ссылаются на один буфер.",
            "Не компилируется, потому что объявлен деструктор."
        },
        2,
        "Неявный конструктор копирования делает поэлементную копию полей: копируется значение "
        "указателя data, а не массив, поэтому a и b ссылаются на один буфер и оба считают себя "
        "его владельцем. Объявленный деструктор не запрещает неявное копирование (такое "
        "поведение лишь устарело), так что код компилируется."
    });

    questions.push_back({
        STAGE_1,
        "Общие данные",
        "Сразу после записи b.data[0] = 99, до разрушения объектов, какое значение хранится в a.data[0]?",
        rawArrayCode,
        {
            "99, потому что оба указателя ведут к одному элементу.",
            "10, потому что b всегда имеет независимый буфер.",
            "Такая запись не компилируется."
        },
        1,
        "a.data и b.data хранят один и тот же адрес, поэтому запись через b.data[0] меняет "
        "общий элемент, и a.data[0] тоже равно 99. Отдельного буфера у b нет: его никто не создавал."
    });

    questions.push_back({
        STAGE_1,
        "Разрушение объектов",
        "Что неправильно при уничтожении a и b, если их указатели всё ещё содержат один адрес?",
        rawArrayCode,
        {
            "Ничего: delete[] автоматически считает владельцев.",
            "Второй delete[] обязан бросить std::bad_alloc.",
            "Один буфер освобождается повторно, возникает undefined behavior."
        },
        3,
        "Оба деструктора передают в delete[] один и тот же адрес: буфер освобождается дважды "
        "(double free), а это undefined behavior. delete[] не считает владельцев и не бросает "
        "исключений, а стандарт не гарантирует ни падения, ни конкретного вывода."
    });

    questions.push_back({
        STAGE_2,
        "Порядок присваивания",
        "В этом операторе копируются int. Какое утверждение верно?",
        "IntArray& operator=(const IntArray& other) {\n"
        "    int* fresh = other.size ? new int[other.size] : nullptr;\n"
        "    for (std::size_t i = 0; i < other.size; ++i)\n"
        "        fresh[i] = other.data[i];\n"
        "    delete[] data;\n"
        "    data = fresh;\n"
        "    size = other.size;\n"
        "    return *this;\n"
        "}",
        {
            "До new надо выполнить delete[] data, иначе старый буфер утечёт.",
            "Если new бросит, объект сохранится; a = a здесь также безопасно.",
            "Любое a = a вызывает double free, если нет отдельного if."
        },
        2,
        "Копирующий конструктор создаёт новый объект, у которого ещё нет буфера, а копирующее "
        "присваивание заменяет значение уже существующего объекта, поэтому старый буфер нужно "
        "освободить, но только когда готова копия fresh. Если new бросит, data и size ещё не "
        "изменены, а при a = a сначала копируются собственные элементы и лишь потом удаляется "
        "старый буфер, так что значение сохраняется."
    });

    questions.push_back({
        STAGE_2,
        "Правила трёх, пяти и нуля",
        "Какой подход к специальным членам класса корректен?",
        "",
        {
            "Согласовать деструктор, copy и при необходимости move; при владении через RAII-поля предпочесть правило нуля.",
            "Деструктор автоматически запрещает copy и реализует move.",
            "Правило нуля запрещает классу владеть std::vector."
        },
        1,
        "Деструктор, копирование и перемещение должны поддерживать одну политику владения "
        "буфером (правило трёх/пяти); объявленный деструктор не реализует move, а лишь отменяет "
        "неявное объявление move-операций. Если ресурсами владеют RAII-поля, например "
        "std::vector, специальные члены лучше не писать вовсе: это правило нуля."
    });

    questions.push_back({
        STAGE_3,
        "Смысл std::move",
        "Что делает само выражение std::move(a)?",
        "",
        {
            "Немедленно обнуляет все поля a.",
            "Безусловно вызывает перемещающий конструктор.",
            "Даёт xvalue; изменение ресурса выполняет выбранная затем операция."
        },
        3,
        "std::move(a) - это только приведение к T&&: выражение имеет категорию xvalue и само "
        "по себе не меняет a. Ресурс переносит перегрузка, которую затем выберет компилятор "
        "(конструктор или operator= с параметром T&&); если её нет или a константный, может "
        "быть выбрано копирование."
    });

    questions.push_back({
        STAGE_3,
        "Перемещающее присваивание",
        "Предположим, data - единолично владеющий int*, size - размер. Какой план подходит для "
        "operator=(IntArray&& other) с контрактом «self-move сохраняет значение»?",
        "",
        {
            "Если this == &other, вернуть *this. Иначе освободить свой буфер, принять поля other, обнулить other и вернуть *this.",
            "Скопировать поля other, оставив источник без изменений, чтобы оба объекта владели буфером.",
            "Вызвать delete[] other.data, затем скопировать этот адрес в data."
        },
        1,
        "Перемещающее присваивание освобождает свой старый буфер, принимает владение буфером "
        "other и обнуляет источник, чтобы у буфера остался ровно один владелец. Проверка "
        "this == &other нужна для контракта: без неё a = std::move(a) удалило бы собственный "
        "буфер, и объект остался бы пустым."
    });

    questions.push_back({
        STAGE_3,
        "Состояние источника",
        "Контракт учебного IntArray: после перемещения в другой объект источник имеет "
        "size == 0 и data == nullptr. Какие действия допустимы?",
        "",
        {
            "Объект уже уничтожен; использовать его нельзя никак.",
            "Прочитать size, проверить data, присвоить новое значение или уничтожить объект; a[0] читать нельзя.",
            "Прочитать a[0]: прежний элемент обязательно сохранился в источнике."
        },
        2,
        "После перемещения a остаётся живым объектом: деструктор ещё не вызван, поэтому можно "
        "читать size, проверять data, присваивать новое значение и уничтожать объект. Чтение a[0] "
        "нарушает предусловие индексации i < size (здесь size == 0); пустота источника - это "
        "контракт нашего IntArray, а у других типов допустимые действия задаёт их собственный контракт."
    });

    questions.push_back({
        STAGE_4,
        "Сравнение содержимого",
        "a и b независимо владеют буферами {10, 20, 30}. Их адреса различны. "
        "operator== сравнивает размер и элементы. Чему равно a == b?",
        "",
        {
            "false, потому что адреса различаются.",
            "Выражение нельзя вычислить для двух объектов.",
            "true, потому что размеры и значения элементов совпадают."
        },
        3,
        "operator== сравнивает значения (размеры и элементы), а не адреса буферов. У a и b "
        "одинаковые размеры и элементы, поэтому a == b равно true, хотя каждый объект "
        "независимо владеет своим буфером."
    });

    questions.push_back({
        STAGE_4,
        "Константная индексация",
        "Есть две перегрузки ниже. a - обычный массив, c - const-массив, оба размера 3. "
        "Какое утверждение верно?",
        "int& operator[](std::size_t i) { return data[i]; }\n"
        "const int& operator[](std::size_t i) const { return data[i]; }",
        {
            "c[0] = 5 допустимо, потому что метод const.",
            "a[0] = 5 и int x = c[0] допустимы; c[0] = 5 не компилируется.",
            "a[10] = 5 обязательно бросит исключение."
        },
        2,
        "Для обычного a вызывается версия, возвращающая int&, поэтому a[0] = 5 допустимо; для "
        "const-объекта c выбирается const-метод с результатом const int&: читать можно, а "
        "c[0] = 5 не компилируется. operator[] не проверяет индекс, поэтому a[10] нарушает "
        "предусловие индексации и даёт undefined behavior, а не гарантированное исключение."
    });

    return questions;
}

std::string trim(const std::string &line) {
    const std::size_t first = line.find_first_not_of(" \t");
    if (first == std::string::npos) {
        return "";
    }
    const std::size_t last = line.find_last_not_of(" \t");
    return line.substr(first, last - first + 1);
}

int readChoice(const std::string &prompt, int minValue, int maxValue) {
    while (true) {
        std::cout << prompt;
        std::string line;
        if (!std::getline(std::cin, line)) {
            std::cout << '\n';
            return INPUT_ENDED;
        }

        const std::string text = trim(line);
        if (text.size() == 1 && text[0] >= '0' + minValue && text[0] <= '0' + maxValue) {
            return text[0] - '0';
        }

        std::cout << "Неверный ввод. Введите одну цифру от " << minValue
                << " до " << maxValue << ".\n";
    }
}

bool waitForEnter() {
    while (true) {
        std::cout << "Нажмите Enter, чтобы продолжить...";
        std::string line;
        if (!std::getline(std::cin, line)) {
            std::cout << '\n';
            return false;
        }
        if (trim(line).empty()) {
            return true;
        }
    }
}

void printLine() {
    std::cout << "------------------------------------------------------------\n";
}

void printCode(const std::string &code) {
    std::istringstream lines(code);
    std::string line;
    while (std::getline(lines, line)) {
        if (line.empty()) {
            std::cout << '\n';
        } else {
            std::cout << "    " << line << '\n';
        }
    }
}

void showMenu() {
    std::cout << '\n';
    printLine();
    std::cout << "Мастерская памяти\n";
    printLine();
    std::cout << "1 - Начать новую игру\n"
            << "2 - Правила и баллы\n"
            << "0 - Выход\n";
}

void showRules() {
    std::cout << '\n';
    printLine();
    std::cout << "Правила\n";
    printLine();
    std::cout
            << "Игра состоит из 10 вопросов о владении памятью в C++ и идёт по этапам:\n"
            << "  1-3  " << STAGE_1 << "\n"
            << "  4-5  " << STAGE_2 << "\n"
            << "  6-8  " << STAGE_3 << "\n"
            << "  9-10 " << STAGE_4 << "\n"
            << "\n"
            << "У каждого вопроса 3 варианта. Введите одну цифру 1, 2 или 3 и нажмите Enter.\n"
            << "Пробелы вокруг цифры допустимы. Пустая строка, буквы, другие числа\n"
            << "не принимаются: вопрос повторяется, счёт не меняется.\n"
            << "Ответ принимается один раз, затем показываются правильный вариант и пояснение.\n"
            << "\n"
            << "Баллы: верный ответ +" << POINTS_PER_ANSWER << ", неверный 0. Максимум 100.\n"
            << "  90-100  Отличное понимание темы\n"
            << "  70-89   Уверенное понимание материала\n"
            << "  50-69   Базовые знания, рекомендуется повторение\n"
            << "  0-49    Стоит повторить материал\n";
}

std::string gradeText(int score) {
    if (score >= 90) {
        return "Отличное понимание темы";
    }
    if (score >= 70) {
        return "Уверенное понимание материала";
    }
    if (score >= 50) {
        return "Базовые знания, рекомендуется повторение";
    }
    return "Стоит повторить материал";
}

void showResult(int correctCount, int total) {
    const int score = correctCount * POINTS_PER_ANSWER;
    std::cout << '\n';
    printLine();
    std::cout << "Результат\n";
    printLine();
    std::cout << "Правильно: " << correctCount << " из " << total << '\n'
            << "Баллы: " << score << " из " << total * POINTS_PER_ANSWER << '\n'
            << "Оценка: " << gradeText(score) << '\n';
}

void showInterrupted(int answeredCount, int total) {
    std::cout << '\n';
    printLine();
    std::cout << "Игра прервана: ввод закончился.\n"
            << "Отвечено вопросов: " << answeredCount << " из " << total << ".\n"
            << "Игра не завершена, итоговая оценка не выставляется.\n";
}

enum class AnswerResult { Right, Wrong, InputEnded };

AnswerResult askQuestion(const Question &q, int number, int total, int score) {
    std::cout << '\n';
    printLine();
    std::cout << "Этап: " << q.stage << '\n'
            << "Вопрос " << number << "/" << total << ". " << q.title
            << "   (баллы: " << score << ")\n";
    printLine();
    std::cout << q.text << "\n\n";

    if (!q.code.empty()) {
        printCode(q.code);
        std::cout << '\n';
    }

    for (int i = 0; i < 3; ++i) {
        std::cout << i + 1 << ") " << q.options[i] << '\n';
    }
    std::cout << '\n';

    const int choice = readChoice("Ваш ответ (1-3): ", 1, 3);
    if (choice == INPUT_ENDED) {
        return AnswerResult::InputEnded;
    }

    const bool isRight = (choice == q.correctNumber);
    std::cout << '\n' << (isRight ? "Верно!" : "Неверно.") << '\n';
    std::cout << "Правильный ответ: " << q.correctNumber << ") "
            << q.options[q.correctNumber - 1] << '\n';
    std::cout << "Пояснение: " << q.explanation << "\n\n";

    return isRight ? AnswerResult::Right : AnswerResult::Wrong;
}

bool playGame(const std::vector<Question> &questions) {
    const int total = static_cast<int>(questions.size());
    int correctCount = 0;
    int answeredCount = 0;
    bool inputEnded = false;

    for (int i = 0; i < total; ++i) {
        const int score = correctCount * POINTS_PER_ANSWER;
        const AnswerResult result = askQuestion(questions[i], i + 1, total, score);
        if (result == AnswerResult::InputEnded) {
            inputEnded = true;
            break;
        }

        ++answeredCount;
        if (result == AnswerResult::Right) {
            ++correctCount;
        }

        if (!waitForEnter()) {
            inputEnded = true;
            break;
        }
    }

    if (answeredCount < total) {
        showInterrupted(answeredCount, total);
    } else {
        showResult(correctCount, total);
    }
    return !inputEnded;
}

int main() {
    const std::vector<Question> questions = createQuestions();

    while (true) {
        showMenu();
        const int command = readChoice("Выберите пункт (0-2): ", 0, 2);

        if (command == INPUT_ENDED) {
            std::cout << "Ввод закончился. Выход из программы.\n";
            return 0;
        }
        if (command == 0) {
            std::cout << "До встречи!\n";
            return 0;
        }
        if (command == 2) {
            showRules();
        } else if (!playGame(questions)) {
            std::cout << "Выход из программы.\n";
            return 0;
        }
    }
}
