#!/bin/bash

echo "🔍 Проверка комментариев к функциям..."
echo ""

ERRORS=0
TOTAL_FUNCS=0

find . -name "*.c" -type f | while read -r file; do
    echo "📄 $(basename "$file"):"
    
    func_count=0
    missing_count=0
    
    # Читаем файл построчно
    line_num=0
    in_comment=0
    
    while IFS= read -r line; do
        ((line_num++))
        
        # Следим за многострочными комментариями
        if [[ $line =~ "/\*" ]]; then
            in_comment=1
        fi
        if [[ $line =~ "\*/" ]]; then
            in_comment=0
            continue
        fi
        
        # Пропускаем если внутри комментария
        if [[ $in_comment -eq 1 ]]; then
            continue
        fi
        
        # Ищем определения функций (упрощенно)
        if [[ $line =~ ^[[:space:]]*[a-zA-Z_][a-zA-Z0-9_]*[[:space:]]+[a-zA-Z_][a-zA-Z0-9_]*[[:space:]]*\(.*\)[[:space:]]*\{?[[:space:]]*$ ]] && \
           ! [[ $line =~ ";" ]] && \
           ! [[ $line =~ ^[[:space:]]*(typedef|struct|enum|union) ]]; then
            
            ((func_count++))
            ((TOTAL_FUNCS++))
            
            # Извлекаем имя функции
            if [[ $line =~ ([a-zA-Z_][a-zA-Z0-9_]*)[[:space:]]*\( ]]; then
                func_name="${BASH_REMATCH[1]}"
                
                # Проверяем есть ли комментарий в 5 строках перед функцией
                has_comment=0
                for ((i=1; i<=5; i++)); do
                    check_line=$((line_num - i))
                    if [[ $check_line -gt 0 ]]; then
                        prev_line=$(sed -n "${check_line}p" "$file")
                        if [[ $prev_line =~ ^[[:space:]]*(/\*|//) ]]; then
                            has_comment=1
                            break
                        fi
                    fi
                done
                
                if [[ $has_comment -eq 0 ]]; then
                    echo "   ❌ $func_name() (строка $line_num)"
                    ((missing_count++))
                    ((ERRORS++))
                fi
            fi
        fi
    done < "$file"
    
    if [[ $missing_count -gt 0 ]]; then
        echo "   ⚠️  Без комментариев: $missing_count/$func_count"
    elif [[ $func_count -gt 0 ]]; then
        echo "   ✅ Все $func_count функций документированы"
    else
        echo "   ℹ️  Функции не найдены"
    fi
    echo ""
done

echo "📊 ИТОГО:"
echo "   Всего функций: $TOTAL_FUNCS"
echo "   Без комментариев: $ERRORS"

if [[ $ERRORS -eq 0 ]]; then
    echo "✅ Отлично! Все функции имеют комментарии."
    exit 0
else
    echo "❌ Найдено $ERRORS функций без комментариев."
    echo ""
    echo "Чтобы добавить автоматические комментарии, выполни:"
    echo "  make format"
    echo ""
    echo "Чтобы проверить форматирование:"
    echo "  make check"
    exit 1
fi
