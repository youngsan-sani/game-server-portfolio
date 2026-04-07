void CTradeDirector::finishTrade(CPlayer* player, CPlayer* targetPlayer)
{
	// 양측의 거래 목록 아이템 삭제, 대상의 아이템이 들어올 수 있는지는 삭제 후 검사해야하므로 먼저 삭제하고 실패 시 롤백
	auto [tradeItemList, targetTradeItemList]= getTradeItemList(player, targetPlayer);
	if (deleteItemList(player, tradeItemList) != SUCCESS)
	{
		onFinishTrade(player, targetPlayer, TRADE_LIST_DELETE_FAIL);
		return;
	}

	if (deleteItemList(targetPlayer, targetTradeItemList) != SUCCESS)
	{
		// 요청자의 아이템이 먼저 차감되었으므로 롤백
		rollbackDeleteItemList(player, tradeItemList);

		onFinishTrade(player, targetPlayer, TRADE_LIST_DELETE_FAIL);
		return;
	}

	// 대상의 거래 목록이 요청자의 인벤토리에 들어올 수 있는지 확인
	if (canAddNewItemList(player, targetTradeItemList) != SUCCESS)
	{
		rollbackDeleteItemList(player, tradeItemList);
		rollbackDeleteItemList(targetPlayer, targetTradeItemList);
		onFinishTrade(player, targetPlayer, INVENTORY_FULL);
		return;
	}

	// 내 거래 목록이 대상의 인벤토리에 들어갈 수 있는지 확인
	if (canAddNewItemList(targetPlayer, tradeItemList) != CS_ERROR_NONE)
	{
		rollbackDeleteItemList(player, tradeItemList);
		rollbackDeleteItemList(targetPlayer, targetTradeItemList);
		onFinishTrade(player, targetPlayer, TARGET_INVENTORY_FULL);
		return;
	}

	// 양측의 골드 차감
	auto [tradeGold, targetTradeGold] = getTradeGold(player, targetPlayer);
	if (useGold(player, tradeGold) != SUCCESS)
	{
		rollbackDeleteItemList(player, tradeItemList);
		rollbackDeleteItemList(targetPlayer, targetTradeItemList);
		onFinishTrade(player, targetPlayer, NOT_ENOUGH_GOLD);
		return;
	}

	if (useGold(targetPlayer, targetTradeGold) != SUCCESS)
	{
		// 요청자의 골드가 먼저 차감되었으므로 롤백
		rollbackUseGold(player, tradeGold);

		rollbackDeleteItemList(player, tradeItemList);
		rollbackDeleteItemList(targetPlayer, targetTradeItemList);
		onFinishTrade(player, targetPlayer, TARGET_NOT_ENOUGH_GOLD);
		return;
	}

	// 거래 목록을 각각의 인벤토리에 추가
	if (addItemList(player, targetTradeItemList) != SUCCESS)
	{
		rollbackUseGold(player, tradeGold);
		rollbackUseGold(targetPlayer, targetTradeGold);
		rollbackDeleteItemList(player, tradeItemList);
		rollbackDeleteItemList(targetPlayer, targetTradeItemList);
		onFinishTrade(player, targetPlayer, TRADE_LIST_ADD_FAIL);
		return;
	}

	if (addItemList(targetPlayer, tradeItemList) != SUCCESS)
	{
		// 요청자의 인벤토리에 추가된 아이템 삭제
		rollbackAddItemList(player, targetTradeItemList);

		rollbackUseGold(player, tradeGold);
		rollbackUseGold(targetPlayer, targetTradeGold);
		rollbackDeleteItemList(player, tradeItemList);
		rollbackDeleteItemList(targetPlayer, targetTradeItemList);
		onFinishTrade(player, targetPlayer, TRADE_LIST_ADD_FAIL);
		return;
	}

	// 골드 추가는 항상 성공
	addGold(player, targetTradeGold);
	addGold(targetPlayer, tradeGold);

	// 거래 성공 처리 및 거래 상태 초기화
	onFinishTrade(player, targetPlayer, SUCCESS);
}
