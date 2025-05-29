#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <algorithm>
#include <unordered_map>

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
	// Можете дополнить ваш класс нужными полями и методами
    Size print_range_{ 0,0 };
    Size table_range_{ 0,0 };

    std::unordered_map<Position, std::unique_ptr<Cell>, PositionHasher> table_;  //  пока уберем, потом переделаем
    
    // snarugi zadaem i ne parimsya
    void SetSHeetSize(Size size) noexcept; //
    void AddPrintRange(Size size);
     void SetPrintRange(Size size);

    enum class PrintDataType
    {
        VALUE,
        TEXT
    };

    void PrintCellData(std::ostream& output, PrintDataType print_data_type) const;
    CellInterface* GetCellData(Position pos) const;

    std::vector<int> max_rows_indexs_; // raven max znachenijam zapolnenyh № strok    count == col_count 
    std::vector<int> max_col_index_;   // raven max znachenijam zapolnenyh № stolbcov count == row_count 
};//my