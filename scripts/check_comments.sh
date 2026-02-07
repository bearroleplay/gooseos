#!/bin/bash

echo "🔍 Проверка комментариев к функциям..."
echo ""

ERRORS=0
CHECKED=0

# Ищем все .c файлы
find . -name "*.c" -type f | while read -r file; do
    echo "📄 Проверяем: $(basename "$file")"
    
    awk -v filename="$file" '
    BEGIN {
        errors = 0
        checked = 0
        in_comment = 0
        last_was_comment = 0
        last_was_empty = 0
        line_num = 0
    }
    
    {
        line_num++
        line = $0
    }
    
    # Многострочные комментарии
    /\/\*/ { in_comment = 1 }
    /\*\// { 
        in_comment = 0
        last_was_comment = 1
        next
    }
    
    # Внутри комментария
    in_comment {
        last_was_comment = 1
        next
    }
    
    # Пустые строки
    /^[[:space:]]*$/ {
        last_was_empty = 1
        next
    }
    
    # Однострочные комментарии
    /^[[:space:]]*\/\// {
        last_was_comment = 1
        last_was_empty = 0
        next
    }
    
    # Пропускаем директивы препроцессора
    /^[[:space:]]*#/ {
        last_was_comment = 0
        last_was_empty = 0
        next
    }
    
    # Пропускаем объявления типов
    /^[[:space:]]*(typedef|struct|union|enum)/ {
        last_was_comment = 0
        last_was_empty = 0
        next
    }
    
    # Нашли функцию (не прототип)
    /^[[:space:]]*[a-zA-Z_][a-zA-Z0-9_]*[[:space:]]+[a-zA-Z_][a-zA-Z0-9_]*[[:space:]]*\([^)]*\)[[:space:]]*[^{]*$/ && !/;/ {
        checked++
        
        if (last_was_comment == 0) {
            # Извлекаем имя функции
            match(line, /[a-zA-Z_][a-zA-Z0-9_]*[[:space:]]*\(/)
            func_name = substr(line, RSTART, RLENGTH-1)
            gsub(/[[:space:]]+$/, "", func_name)
            
            printf "   ❌ Строка %d: Функция \"%s\" без комментария\n", line_num, func_name
            errors++
        }
        
        last_was_comment = 0
        last_was_empty = 0
        next
    }
    
    # Все остальные строки
    {
        last_was_comment = 0
        last_was_empty = 0
    }
    
    END {
        if (errors > 0) {
            printf "   ⚠️  Найдено %d функций без комментариев\n", errors
        } else if (checked > 0) {
            printf "   ✅ Все %d функций документированы\n", checked
        } else {
            printf "   ℹ️  Функции не найдены\n"
        }
        # Возвращаем количество ошибок
        exit errors
    }
    ' "$file"
    
    if [ $? -ne 0 ]; then
        ERRORS=$((ERRORS + 1))
    fi
    
    CHECKED=$((CHECKED + 1))
    echo ""
done

echo "📊 ИТОГО:"
echo "   Проверено файлов: $CHECKED"
if [ $ERRORS -eq 0 ]; then
    echo "   ✅ Все функции имеют комментарии!"
    exit 0
else
    echo "   ❌ Найдено проблем: $ERRORS"
    echo ""
    echo "Рекомендации:"
    echo "1. Запусти: make format  (добавит автоматические комментарии)"
    echo "2. Отредактируй автоматические комментарии вручную"
    echo "3. Запусти: make check   (проверь снова)"
    exit 1
fi
