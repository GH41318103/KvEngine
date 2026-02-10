/**
 * @file iterator.h
 * @brief 迭代器接口定義
 * @details 提供鍵值對的迭代訪問功能
 */

#ifndef KVENGINE_ITERATOR_H
#define KVENGINE_ITERATOR_H

#include <string>
#include <map>
#include <memory>

namespace kvengine {

/**
 * @class Iterator
 * @brief 迭代器抽象基類
 * @details 定義了遍歷鍵值對的標準接口
 */
class Iterator {
public:
    Iterator() = default;
    virtual ~Iterator() = default;
    
    /**
     * @brief 檢查迭代器是否有效
     * @return 有效返回 true，已到末尾返回 false
     */
    virtual bool valid() const = 0;
    
    /**
     * @brief 移動到下一個元素
     */
    virtual void next() = 0;
    
    /**
     * @brief 獲取當前鍵
     * @return 當前鍵的字符串
     */
    virtual std::string key() const = 0;
    
    /**
     * @brief 獲取當前值
     * @return 當前值的字符串
     */
    virtual std::string value() const = 0;
    
    /**
     * @brief 定位到指定鍵
     * @param target 目標鍵
     */
    virtual void seek(const std::string& target) = 0;
    
    /**
     * @brief 定位到第一個元素
     */
    virtual void seek_to_first() = 0;
};

/**
 * @class MapIterator
 * @brief 基於 std::map 的迭代器實現
 * @details 支持前綴過濾的迭代器，通過保存 map 的副本來保證迭代安全性
 */
class MapIterator : public Iterator {
public:
    /**
     * @brief 構造函數
     * @param data 原始數據 map
     * @param prefix 可選的前綴過濾字符串
     */
    explicit MapIterator(const std::map<std::string, std::string>& data,
                        const std::string& prefix = "");
    
    /**
     * @brief 檢查迭代器是否有效
     * @return 如果當前位置有效且匹配前綴（如果有），返回 true
     */
    bool valid() const override;

    /**
     * @brief 移動到下一個元素
     * @details 如果設置了前綴，將跳過不匹配前綴的元素
     */
    void next() override;

    /**
     * @brief 獲取當前鍵
     * @return 當前鍵的字符串副本
     */
    std::string key() const override;

    /**
     * @brief 獲取當前值
     * @return 當前值的字符串副本
     */
    std::string value() const override;

    /**
     * @brief 定位到指定鍵
     * @details 將迭代器移動到大於或等於 target 的第一個位置
     * @param target 目標鍵
     */
    void seek(const std::string& target) override;

    /**
     * @brief 定位到第一個元素
     * @details 重置迭代器到起始位置
     */
    void seek_to_first() override;
    
private:
    std::shared_ptr<std::map<std::string, std::string>> data_copy_; ///< 數據副本，保證迭代穩定性
    std::map<std::string, std::string>::const_iterator iter_;       ///< 當前內部迭代器
    std::map<std::string, std::string>::const_iterator end_;        ///< 結束迭代器
    std::string prefix_;                                            ///< 前綴過濾條件
    
    /**
     * @brief 檢查當前元素是否匹配前綴
     * @return 匹配返回 true，否則返回 false
     */
    bool matches_prefix() const;
};

} // namespace kvengine

#endif // KVENGINE_ITERATOR_H
