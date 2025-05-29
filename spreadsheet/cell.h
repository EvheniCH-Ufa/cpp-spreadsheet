#pragma once

#include "common.h"
#include "formula.h"
//#include "sheet.h"

#include <functional>
#include <unordered_set>

#include <optional>

class Sheet;

struct PositionHasher {
    size_t  operator()(const Position& pos) const noexcept
    {
        return pos.col + pos.row * 16387; // 16387 - pervoe prostoe chislo bolshe 16384 - tochno bez kollizii; 
    };
};
using PositionUSet = std::unordered_set<Position, PositionHasher>;


class Cell : public CellInterface
{
    public:
        Cell(Sheet& sheet);
        ~Cell();

        void Clear();

        Value GetValue() const override;
        std::string GetText() const override;

        std::vector<Position> GetReferencedCells() const override;
        //vozvrashchaet u_set zavis cell
        PositionUSet GetDependentCell() const;

        void Set(Position pos, std::string text); //udaleno???

        void AddDependentCell(Position pos);

        //deletim pos iz spiska zavis cells
        void DelDependentCell(Position pos);
        //deletim pos iz spiska vliyayushchih cells
        void DelInfluencingCell(Position pos);

        void ResetValues(PositionUSet& reseted_list);                                       //sbros all zavis value

        bool CheckCycles(PositionUSet& is_checked, const PositionUSet& prev_cells) const; //proverka na ciklichnost'


    private:
        class Impl;                     // тут три метода будут виртуал = 0, м.б. нет, сет не нужен. он в целле
        class EmptyImpl;                // тут они долны пустышку выдавать
        class TextImpl;                 // тут согласно расписанию
        class FormulaImpl;              // тут согласно расписанию

        std::unique_ptr<Impl> impl_;    // тут подсталяем то или иное. круто, осталось сделать



        // Добавьте поля и методы для связи с таблицей, проверки циклических
        // зависимостей, графа зависимостей и т. д.
       // HOBOE

        void ResetValue();                                                 //sbros curr value
        bool is_referenced = false;

        std::unordered_set<Position, PositionHasher> dependent_cells_;     // spisok zavisimyh cell's
        std::unordered_set<Position, PositionHasher> influencing_cells_;   // spisok vliyayushchich cell's

        std::vector<Position> GetDependentCells() const;

        mutable std::optional<FormulaInterface::Value> value_cache_;       // hranym vichislennoe znachenie
        Position pos_;

        Sheet& sheet_;
};//my